#!/usr/bin/env python3
from http.server import HTTPServer, SimpleHTTPRequestHandler
import urllib.request
import urllib.error

REGISTRY = "https://registry.wasmer.io/graphql"

class Handler(SimpleHTTPRequestHandler):
    def _set_common_headers(self):
        # Needed by @wasmer/sdk (SharedArrayBuffer + workers) in modern browsers
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")
        self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
        self.send_header("Cache-Control", "no-store")

    def end_headers(self):
        self._set_common_headers()
        super().end_headers()

    def do_OPTIONS(self):
        # For completeness (some browsers may preflight depending on headers)
        if self.path == "/registry/graphql":
            self.send_response(204)
            self.send_header("Access-Control-Allow-Origin", self.headers.get("Origin", "*"))
            self.send_header("Access-Control-Allow-Methods", "POST, OPTIONS")
            self.send_header("Access-Control-Allow-Headers", "content-type, authorization")
            self.end_headers()
            return
        return super().do_OPTIONS()

    def do_POST(self):
        if self.path != "/registry/graphql":
            self.send_error(404, "Not Found")
            return

        length = int(self.headers.get("Content-Length", "0"))
        body = self.rfile.read(length)

        req = urllib.request.Request(
            REGISTRY,
            data=body,
            method="POST",
            headers={
                # Keep it minimal; don't forward browser-only headers
                "Content-Type": self.headers.get("Content-Type", "application/json"),
            },
        )

        try:
            with urllib.request.urlopen(req) as resp:
                data = resp.read()
                self.send_response(resp.status)
                self.send_header("Content-Type", resp.headers.get("Content-Type", "application/json"))
                self.send_header("Access-Control-Allow-Origin", self.headers.get("Origin", "*"))
                self.end_headers()
                self.wfile.write(data)
        except urllib.error.HTTPError as e:
            data = e.read()
            self.send_response(e.code)
            self.send_header("Content-Type", e.headers.get("Content-Type", "application/json"))
            self.send_header("Access-Control-Allow-Origin", self.headers.get("Origin", "*"))
            self.end_headers()
            self.wfile.write(data)

if __name__ == "__main__":
    HTTPServer(("localhost", 8003), Handler).serve_forever()
    