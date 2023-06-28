import http.server
import socketserver

PORT = 6000

class MyHandler(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == '/final-project.bin':
            # Logic to handle the OTA update
            with open('final-project.bin', 'rb') as f:
                data = f.read()
                self.send_response(200)
                self.send_header('Content-type', 'application/octet-stream')
                self.send_header('Content-Length', len(data))  # Set the Content-Length header
                self.end_headers()
                self.wfile.write(data)
        else:
            # Serve other files
            super().do_GET()
            
with socketserver.TCPServer(("", PORT), MyHandler) as httpd:
    print("serving at port", PORT)
    httpd.serve_forever()
