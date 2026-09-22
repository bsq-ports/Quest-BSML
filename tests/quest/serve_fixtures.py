"""Local fixtures for Quest tests. Run adb reverse tcp:8765 tcp:8765 first."""
import argparse
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import time
from urllib.parse import urlsplit

ROOT = Path(__file__).resolve().parent / "fixtures"
FILES = {p.name: p.read_bytes() for p in ROOT.iterdir() if p.suffix in (".gif", ".png")}

class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        path = urlsplit(self.path).path
        slow = path.startswith("/slow/")
        name = path[len("/slow/"):] if slow else path[1:]
        if name not in FILES:
            self.send_error(404)
            return
        if slow:
            time.sleep(2)
        body = FILES[name]
        self.send_response(200)
        self.send_header("Content-Type", "image/gif" if name.endswith(".gif") else "image/png")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        try:
            self.wfile.write(body)
        except (BrokenPipeError, ConnectionResetError, ConnectionAbortedError):
            pass  # Expected when a request is cancelled by the tests.

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", type=int, default=8765)
    args = parser.parse_args()
    print(f"Serving Quest fixtures at http://127.0.0.1:{args.port}", flush=True)
    ThreadingHTTPServer(("127.0.0.1", args.port), Handler).serve_forever()
