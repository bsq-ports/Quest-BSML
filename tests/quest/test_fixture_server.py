import threading
import time
import unittest
from urllib.error import HTTPError
from urllib.request import urlopen
from http.server import ThreadingHTTPServer
from serve_fixtures import FILES, Handler

class FixtureServerTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
        cls.thread = threading.Thread(target=cls.server.serve_forever, daemon=True)
        cls.thread.start()
        cls.base = f"http://127.0.0.1:{cls.server.server_port}"

    @classmethod
    def tearDownClass(cls):
        cls.server.shutdown()
        cls.server.server_close()
        cls.thread.join()

    def test_exact_bytes_and_no_cache(self):
        for name, data in FILES.items():
            with urlopen(self.base + "/" + name + "?run=42", timeout=5) as response:
                self.assertEqual(response.read(), data)
                self.assertEqual(response.headers["Cache-Control"], "no-store")

    def test_slow_response(self):
        start = time.monotonic()
        with urlopen(self.base + "/slow/b.gif", timeout=5) as response:
            self.assertEqual(response.read(), FILES["b.gif"])
        self.assertGreaterEqual(time.monotonic() - start, 1.9)

    def test_no_arbitrary_files(self):
        for path in ("/../serve_fixtures.py", "/slow/../../README.md", "/missing.gif"):
            with self.assertRaises(HTTPError) as error:
                urlopen(self.base + path, timeout=5)
            self.assertEqual(error.exception.code, 404)

if __name__ == "__main__":
    unittest.main()
