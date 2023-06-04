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

        value1 = int(data.get('value1', [0])[0])
        value2 = int(data.get('value2', [0])[0])
        value3 = float(data.get('temperature', ['0'])[0])
        value4 = float(data.get('humidity', ['0'])[0])
        print(f"Received data: value1 = {value1}, value2 = {value2} value3 = {value3} value4 = {value4}")

        #write value1 and value2 to jsonfile named data.json
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
