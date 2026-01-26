#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/DiagnosticOptions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/CompilerInvocation.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/FileSystemOptions.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/VirtualFileSystem.h"
#include "llvm/Support/raw_ostream.h"

#ifdef __EMSCRIPTEN__
const char *Triple = "wasm32-unknown-emscripten";
#else
const char *Triple = "";
#endif

using namespace clang;

static llvm::cl::opt<std::string> OptStd(
    "std", llvm::cl::desc("C++ standard (e.g. c++20, c++23)"),
    llvm::cl::init("c++20"));

static llvm::cl::opt<std::string> OptResultName(
    "result",
    llvm::cl::desc("Name of constexpr variable or constexpr/consteval function"),
    llvm::cl::init("__result"));

static llvm::cl::opt<bool> OptStdin(
    "stdin", llvm::cl::desc("Read source from stdin (default)"),
    llvm::cl::init(false));

static llvm::cl::opt<std::string> OptExpr(
    "expr",
    llvm::cl::desc("Wrap expression as: constexpr auto <result> = (<expr>);"),
    llvm::cl::init(""));

static llvm::cl::opt<std::string> OptPath(
    "path",
    llvm::cl::desc("Path to input file inside the virtual filesystem"),
    llvm::cl::init("/input.cpp"));

static llvm::cl::opt<bool> OptCheckOnly(
    "check-only",
    llvm::cl::desc("Only check compilation (for static_assert tests), don't extract result"),
    llvm::cl::init(false));

static llvm::cl::list<std::string> OptIncludes(
    "include-dir",
    llvm::cl::desc("Additional include directories (-I)"),
    llvm::cl::ZeroOrMore);

static std::string readAllStdin()
{
    std::string S;
    char buf[4096];
    while (true)
    {
        size_t n = fread(buf, 1, sizeof(buf), stdin);
        if (n == 0)
            break;
        S.append(buf, buf + n);
    }
    return S;
}

static std::string apintToString(const llvm::APInt &I, bool Signed)
{
    llvm::SmallString<64> S;
    I.toString(S, /*Radix=*/10, /*Signed=*/Signed,
               /*formatAsCLiteral=*/false, /*UpperCase=*/true,
               /*InsertSeparators=*/false);
    return std::string(S.str());
}

static std::string apsintToString(const llvm::APSInt &I)
{
    llvm::SmallString<64> S;
    I.toString(S, /*Radix=*/10);
    return std::string(S.str());
}

struct FindAndEval : public RecursiveASTVisitor<FindAndEval>
{
    ASTContext &Ctx;
    std::string Name;

    bool Found = false;
    std::string ResultText;
    std::string Error;

    FindAndEval(ASTContext &C, std::string N) : Ctx(C), Name(std::move(N)) {}

    std::string formatAPValue(const APValue &V, QualType Ty)
    {
        if (V.isInt())
            return apintToString(V.getInt(), Ty->isSignedIntegerType());

        if (V.isFloat())
        {
            llvm::SmallString<32> S;
            V.getFloat().toString(S);
            return std::string(S.str());
        }

        if (V.isComplexInt())
        {
            return "(" + apsintToString(V.getComplexIntReal()) + "," +
                   apsintToString(V.getComplexIntImag()) + ")";
        }

        if (V.isComplexFloat())
        {
            llvm::SmallString<32> R, I;
            V.getComplexFloatReal().toString(R);
            V.getComplexFloatImag().toString(I);
            return "(" + std::string(R.str()) + "," + std::string(I.str()) + ")";
        }

        // Minimal "text" support: try constant char array (e.g. constexpr char[]).
        if (Ty->isArrayType() && V.isArray())
        {
            std::string Out;
            unsigned N = V.getArraySize();
            for (unsigned i = 0; i < N; i++)
            {
                const APValue &Elt = V.getArrayInitializedElt(i);
                if (!Elt.isInt())
                {
                    Out.clear();
                    break;
                }
                unsigned char c = (unsigned char)Elt.getInt().getZExtValue();
                if (c == 0)
                    break;
                Out.push_back((char)c);
            }
            if (!Out.empty())
                return Out;
        }

        return {};
    }

    bool VisitVarDecl(VarDecl *VD)
    {
        if (Found)
            return true;
        if (!VD || !VD->isConstexpr())
            return true;
        if (VD->getName() != Name)
            return true;

        Found = true;

        if (!VD->hasInit())
        {
            Error = "constexpr variable has no initializer";
            return true;
        }

        // 1) Prefer Clang's cached evaluated value (better diagnostics behavior).
        if (const APValue *Cached = VD->getEvaluatedValue())
        {
            ResultText = formatAPValue(*Cached, VD->getType());
            if (ResultText.empty())
                Error = "unsupported constexpr value kind";
            return true;
        }

        // 2) Fall back to forcing evaluation.
        const Expr *Init = VD->getInit();
        Expr::EvalResult ER;
        if (!Init->EvaluateAsRValue(ER, Ctx))
        {
            // At this point, Clang often already emitted diagnostics into your
            // DiagnosticsEngine. Keep this error as a fallback.
            Error = "constexpr evaluation failed (see diagnostics)";
            return true;
        }

        ResultText = formatAPValue(ER.Val, VD->getType());
        if (ResultText.empty())
            Error = "unsupported constexpr value kind";
        return true;
    }

    bool VisitFunctionDecl(FunctionDecl *FD)
    {
        if (Found)
            return true;
        if (!FD || FD->getName() != Name)
            return true;
        if (!(FD->isConstexpr() || FD->isConsteval()))
            return true;
        if (!FD->doesThisDeclarationHaveABody())
        {
            Error = "constexpr function has no body";
            Found = true;
            return true;
        }
        if (FD->param_size() != 0)
        {
            Error = "constexpr function must take 0 params";
            Found = true;
            return true;
        }

        // Build a DeclRefExpr to the function, then a CallExpr, and constant-evaluate it.
        DeclarationNameInfo DNI(FD->getDeclName(), FD->getLocation());
        auto *Callee = DeclRefExpr::Create(
            Ctx, NestedNameSpecifierLoc(), SourceLocation(),
            FD, /*RefersToEnclosingVariableOrCapture=*/false,
            DNI, FD->getType(), VK_LValue);

        llvm::SmallVector<Expr *, 0> Args;
        QualType RetTy = FD->getReturnType();

        auto *Call = CallExpr::Create(
            Ctx,
            /*Fn=*/Callee,
            /*Args=*/Args,
            /*Ty=*/RetTy,
            /*VK=*/VK_PRValue,
            /*RParenLoc=*/SourceLocation(),
            /*FPFeatures=*/FPOptionsOverride(),
            /*MinNumArgs=*/0,
            /*UsesADL=*/CallExpr::NotADL);

        Expr::EvalResult ER;
        if (!Call->EvaluateAsRValue(ER, Ctx))
        {
            Error = "failed to evaluate constexpr function call";
            Found = true;
            return true;
        }

        Found = true;
        ResultText = formatAPValue(ER.Val, RetTy);
        if (ResultText.empty() && Error.empty())
            Error = "unsupported constexpr value kind";
        return true;
    }
};

class EvalConsumer final : public ASTConsumer
{
public:
    std::string Name;
    bool &Ok;
    std::string &Result;
    std::string &Err;

    EvalConsumer(std::string N, bool &OkRef, std::string &ResRef, std::string &ErrRef)
        : Name(std::move(N)), Ok(OkRef), Result(ResRef), Err(ErrRef) {}

    void HandleTranslationUnit(ASTContext &Ctx) override
    {
        FindAndEval V(Ctx, Name);
        V.TraverseDecl(Ctx.getTranslationUnitDecl());

        if (!V.Found)
        {
            Ok = false;
            Err = "did not find constexpr variable or constexpr/consteval function named '" + Name + "'";
            return;
        }
        if (!V.Error.empty())
        {
            Ok = false;
            Err = V.Error;
            return;
        }
        Ok = true;
        Result = V.ResultText;
    }
};

class EvalAction final : public ASTFrontendAction
{
public:
    std::string ResultName;
    bool Ok = false;
    std::string ResultText;
    std::string ErrorText;

    explicit EvalAction(std::string N) : ResultName(std::move(N)) {}

    std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                   llvm::StringRef) override
    {
        return std::make_unique<EvalConsumer>(ResultName, Ok, ResultText, ErrorText);
    }
};

int main(int argc, const char **argv)
{
    // Reset LLVM command-line option state so callMain() can be invoked
    // repeatedly from JS without "option already set" errors.
    llvm::cl::ResetAllOptionOccurrences();
    llvm::cl::ParseCommandLineOptions(argc, argv);

    const std::string Path = OptPath;

    // Decide whether we are supplying the file contents ourselves.
    bool ProvideContents = !OptExpr.empty();
    if (OptStdin && OptExpr.empty())
    {
        // Only read stdin if the user explicitly wants it; for browser use, prefer --path.
        // You can keep OptStdin default false if you want.
        ProvideContents = true;
    }

    IntrusiveRefCntPtr<llvm::vfs::InMemoryFileSystem> MemFS(
        new llvm::vfs::InMemoryFileSystem());

    if (ProvideContents)
    {
        std::string Code = OptExpr.empty()
                               ? readAllStdin()
                               : ("constexpr auto " + OptResultName + " = (" + OptExpr + ");\n");

        MemFS->addFile(Path, 0, llvm::MemoryBuffer::getMemBufferCopy(Code, Path));
    }
    // Diagnostics to string.
    std::string DiagBuf;
    llvm::raw_string_ostream DiagOS(DiagBuf);
    DiagnosticOptions DiagOpts;
    auto DiagClient = std::make_unique<TextDiagnosticPrinter>(DiagOS, DiagOpts);

    IntrusiveRefCntPtr<DiagnosticIDs> DiagID(new DiagnosticIDs());
    IntrusiveRefCntPtr<DiagnosticsEngine> Diags(
        new DiagnosticsEngine(DiagID, DiagOpts, DiagClient.get(),
                              /*ShouldOwnClient=*/false));

    // Build invocation args (stable storage).
    std::string StdArg = "-std=" + OptStd;
    llvm::SmallVector<const char *, 16> Args;
    Args.push_back("-x");
    Args.push_back("c++");
    Args.push_back(StdArg.c_str());
    Args.push_back("-fsyntax-only");
#ifdef __EMSCRIPTEN__
    Args.push_back("-triple");
    Args.push_back(Triple);
    // Disable auto-detection of include paths (won't work inside WASM)
    Args.push_back("-nobuiltininc");
    Args.push_back("-nostdsysteminc");
    // Explicit include paths in correct #include_next order:
    //   libc++ → Clang builtins → musl C headers
    Args.push_back("-isystem");
    Args.push_back("/sysroot/include/c++/v1");
    Args.push_back("-isystem");
    Args.push_back("/sysroot/lib/clang/21/include");
    Args.push_back("-isystem");
    Args.push_back("/sysroot/include");
    Args.push_back("-Wno-macro-redefined");
    Args.push_back("-Wno-nullability-completeness");
    Args.push_back("-Wno-nullability-extension");

    // Args.push_back("-ferror-limit");
    // Args.push_back("0");
    // Args.push_back("-fconstexpr-backtrace-limit");
    // Args.push_back("0");
    // Args.push_back("-fconstexpr-steps");
    // Args.push_back("100000000");
#else
    // Native build: let Clang find system headers automatically
#endif

    // Forward user-specified include directories.
    for (const auto &Dir : OptIncludes)
    {
        Args.push_back("-I");
        Args.push_back(Dir.c_str());
    }

    Args.push_back(Path.c_str());

    auto Invocation = std::make_shared<CompilerInvocation>();
    CompilerInvocation::CreateFromArgs(*Invocation, Args, *Diags);

    Invocation->getDiagnosticOpts().ErrorLimit = 0;
    // Invocation->getLangOpts().ConstexprBacktraceLimit = 0;
    // Invocation->getLangOpts().ConstexprStepLimit = 100000000;

    // Create compiler instance with this invocation.
    CompilerInstance CI(Invocation);
    CI.setDiagnostics(Diags.get());

    auto &DE = CI.getDiagnostics();

    // Disable the nullability warnings (covers the ones you pasted)
    DE.setSeverityForGroup(clang::diag::Flavor::WarningOrError,
                        "nullability-completeness",
                        clang::diag::Severity::Ignored,
                        clang::SourceLocation());

    // Sometimes also needed:
    DE.setSeverityForGroup(clang::diag::Flavor::WarningOrError,
                        "nullability-extension",
                        clang::diag::Severity::Ignored,
                        clang::SourceLocation());
    DE.setSeverityForGroup(clang::diag::Flavor::WarningOrError,
                    "macro-redefined",
                    clang::diag::Severity::Ignored,
                    clang::SourceLocation());

    IntrusiveRefCntPtr<llvm::vfs::FileSystem> BaseFS = llvm::vfs::getRealFileSystem();
    IntrusiveRefCntPtr<llvm::vfs::OverlayFileSystem> OFS(new llvm::vfs::OverlayFileSystem(BaseFS));
    OFS->pushOverlay(MemFS);

    // Use in-memory FS.
    FileSystemOptions FSOpts;
    CI.setFileManager(new FileManager(FSOpts, OFS));
    CI.createSourceManager(CI.getFileManager());

    bool Success;
    std::string ResultText, ErrorText;
    bool ActionOk;

    if (OptCheckOnly)
    {
        // Check-only mode: just run syntax/semantic analysis (triggers static_assert).
        SyntaxOnlyAction SOA;
        Success = CI.ExecuteAction(SOA);
        ActionOk = Success && !Diags->hasErrorOccurred();
    }
    else
    {
        EvalAction Action(OptResultName);
        Success = CI.ExecuteAction(Action);
        ActionOk = Success && Action.Ok && !Diags->hasErrorOccurred();
        ResultText = Action.ResultText;
        ErrorText = Action.ErrorText;
    }

    DiagOS.flush();

    llvm::json::Object Out;
    Out["ok"] = ActionOk;
    Out["result"] = ResultText;
    Out["error"] = ErrorText;
    Out["diagnostics"] = DiagBuf;

    llvm::outs() << llvm::json::Value(std::move(Out)) << "\n";
    return ActionOk ? 0 : 1;
}