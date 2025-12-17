import argparse
import http.server
import json
import os
import random
from http import HTTPStatus

ITERATION_THRESHOLD = 60
class Handler(http.server.SimpleHTTPRequestHandler):
    motor_value = 0
    last_value = 0
    up_trend = True
    it = 0
    def do_GET(self):
        if self.path.startswith('/MOTOR_STATUS'):
            # produce a testable numeric value (incremental with some noise)
            last_value = Handler.motor_value
            Handler.motor_value = Handler.motor_value + random.choice([0,0,10,1]) if Handler.up_trend else Handler.motor_value - random.choice([0,0,10,1])
            value = Handler.motor_value + random.choice([0,0,3,-3])  # small jitter
            Handler.up_trend = Handler.it < ITERATION_THRESHOLD/2 if value > 0 else True
            value = int(max(0, value) % 500)
            pwm = (value - last_value) / 2
            text = str(value) + ' ' + str(pwm)
            body = text.encode('utf-8')
            self.send_response(HTTPStatus.OK)
            self.send_header('Content-Type', 'text/plain; charset=utf-8')
            self.send_header('Content-Length', str(len(body)))
            self.end_headers()
            self.wfile.write(body)
            Handler.it += 1
            if Handler.it >= ITERATION_THRESHOLD:
                Handler.it = 0
            return None
        # otherwise serve static files from given directory
        return super().do_GET()

def run(port, root):
    os.chdir(root)
    server = http.server.ThreadingHTTPServer(('0.0.0.0', port), Handler)
    print(f"Serving ` {root} ` on http://127.0.0.1:{port} (CTRL+C to stop)")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()

if __name__ == '__main__':
    p = argparse.ArgumentParser()
    p.add_argument('--port', type=int, default=8000)
    p.add_argument('--root', type=str, default='.')
    args = p.parse_args()
    run(args.port, args.root)