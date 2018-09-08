TEST_LIST = []


def test(func):
    '''
    Decorator to automatically register a function to execute at integration-
    test time.
    '''
    global TEST_LIST
    TEST_LIST.append(func)
    return func


def run_tests(node):
    for t in TEST_LIST:
        t(node)
