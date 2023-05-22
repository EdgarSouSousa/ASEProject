import http.server
import socketserver
from urllib.parse import parse_qs

PORT = 8000

class CustomRequestHandler(http.server.SimpleHTTPRequestHandler):
    def do_POST(self):
        content_length = int(self.headers['Content-Length'])
        post_data = self.rfile.read(content_length)
        data = parse_qs(post_data.decode('utf-8'))

        value1 = int(data.get('value1', [0])[0])
        value2 = int(data.get('value2', [0])[0])

        print(f"Received data: value1 = {value1}, value2 = {value2}")

        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()
        self.wfile.write(b"OK")

Handler = CustomRequestHandler
httpd = socketserver.TCPServer(("", PORT), Handler)
print(f"Serving on port {PORT}")

httpd.serve_forever()
