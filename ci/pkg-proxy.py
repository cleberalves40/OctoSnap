#!/usr/bin/env python3
"""HTTP server for the anita CI jobs, reachable from the VM as 10.0.2.2.

  /src.tgz      the source tarball from the serve directory
  /pkgsrc/...   pull-through cache of https://cdn.netbsd.org/pub/pkgsrc/...

Downloading over QEMU user networking from inside an emulated guest was
slow and dropped connections (pkg_add spent over two hours on NetBSD/i386);
the host fetches each file once over its own network instead.

usage: pkg-proxy.py <serve-dir> <cache-dir> [port]
"""

import http.server
import os
import shutil
import sys
import urllib.error
import urllib.request

UPSTREAM = "https://cdn.netbsd.org/pub/pkgsrc/"


class Handler(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):
        if not self.path.startswith("/pkgsrc/"):
            return super().do_GET()

        rel = self.path[len("/pkgsrc/"):].split("?", 1)[0]
        if ".." in rel.split("/"):
            return self.send_error(400)
        # Directory listings are cached too; pkg_add reads them to match versions.
        cached = os.path.join(CACHE, rel if not rel.endswith("/") else rel + "index.html")
        if not os.path.exists(cached):
            try:
                with urllib.request.urlopen(UPSTREAM + rel, timeout=300) as resp:
                    os.makedirs(os.path.dirname(cached), exist_ok=True)
                    with open(cached + ".part", "wb") as out:
                        shutil.copyfileobj(resp, out)
                os.replace(cached + ".part", cached)
            except urllib.error.HTTPError as e:
                return self.send_error(e.code)
            except OSError:
                return self.send_error(502)

        self.send_response(200)
        self.send_header("Content-Length", str(os.path.getsize(cached)))
        self.send_header("Content-Type", "text/html" if cached.endswith(".html")
                         else "application/octet-stream")
        self.end_headers()
        with open(cached, "rb") as f:
            shutil.copyfileobj(f, self.wfile)

    def log_message(self, fmt, *args):
        pass


if __name__ == "__main__":
    serve, CACHE = sys.argv[1], sys.argv[2]
    port = int(sys.argv[3]) if len(sys.argv) > 3 else 8000
    os.chdir(serve)
    http.server.ThreadingHTTPServer(("127.0.0.1", port), Handler).serve_forever()
