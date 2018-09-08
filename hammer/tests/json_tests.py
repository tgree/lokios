import integration_test

def test_paths(node, paths):
    for p in paths:
        print "hammer: Validating json for '%s'" % p
        node.get(p).jdict()


@integration_test.test
def test_fixed_json_nodes(node):
    test_paths(node, ['/', '/acpi', '/net', '/pci'])


@integration_test.test
def test_acpi_nodes(node):
    acpi_root = node.get('/acpi').jdict()
    paths     = ['/acpi/%s' % t for t in acpi_root.tables]
    test_paths(node, paths)


@integration_test.test
def test_net_nodes(node):
    net_root   = node.get('/net').jdict()
    interfaces = [intf.name for intf in net_root.interfaces]
    for intf in interfaces:
        paths = ['/net/%s' % intf, '/net/%s/tcp' % intf]
        test_paths(node, paths)


@integration_test.test
def test_pci_nodes(node):
    pci_root = node.get('/pci').jdict()
    domains  = [int(d, 16) for d in pci_root.domains]
    for d in domains:
        domain_root = node.get('/pci/%04X' % d).jdict()
        paths       = ['/pci/%04X' % d]
        paths      += ['/pci/%04X/%s' % (d, dev.slot)
                       for dev in domain_root.devices]
        test_paths(node, paths)
