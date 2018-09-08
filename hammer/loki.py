import requests
import time


class Node(object):
    def __init__(self, ip, port):
        self.ip       = ip
        self.port     = port
        self.base_url = 'http://%s:%u/' % (self.ip, self.port)
        self.session  = requests.Session()

    def connect(self, timeout=10):
        start = time.time()
        while time.time() - start < timeout:
            try:
                self.get('/')
                return True
            except (requests.exceptions.Timeout,
                    requests.exceptions.ConnectionError):
                pass
        return False

    def url_for_path(self, path):
        if path.startswith('/'):
            return self.base_url + path[1:]
        return self.base_url + path

    def get(self, path, timeout=0.5):
        return self.session.get(self.url_for_path(path), timeout=timeout)

    def post(self, path, json=None, timeout=1):
        return self.session.post(self.url_for_path(path), json=json,
                                 timeout=timeout)

    def shutdown(self, exit_code=3):
        self.post('/', json={'state'     : 'stopped',
                             'exit_code' : '%u' % exit_code})

    def __repr__(self):
        return "loki.Node('%s', %s)" % (self.ip, self.port)
