import http.server
import socketserver
from urllib.parse import parse_qs
import json

PORT = 5000

class CustomRequestHandler(http.server.SimpleHTTPRequestHandler):
    def do_POST(self):
        content_length = int(self.headers['Content-Length'])
        post_data = self.rfile.read(content_length)
        data = parse_qs(post_data.decode('utf-8'))

        print(f"Received data: {data}")

        # Write the data dictionary to a JSON file named data.json
        with open('data.json', 'w') as outfile:
            json.dump(data, outfile)

        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()
        self.wfile.write(b"OK")

Handler = CustomRequestHandler
httpd = socketserver.TCPServer(("", PORT), Handler)
print(f"Serving on port {PORT}")

httpd.serve_forever()

