import integration_test

def test_paths(node, paths):
    for p in paths:
        print "hammer: Validating json for '%s'" % p
        node.get(p).jdict()


@integration_test.test
def test_fixed_json_nodes(node):
    test_paths(node, ['/', '/acpi', '/mm', '/net', '/pci', '/vmm'])


@integration_test.test
def test_acpi_nodes(node):
    acpi_root = node.get('/acpi').jdict()
    paths     = ['/acpi/%s' % t for t in acpi_root.tables]
    test_paths(node, paths)


@integration_test.test
def test_mm_nodes(node):
    mm_root = node.get('/mm').jdict()
    paths   = ['/mm/e820']
    test_paths(node, paths)


@integration_test.test
def test_net_nodes(node):
    net_root   = node.get('/net').jdict()
    interfaces = [intf.name for intf in net_root.interfaces]
    for intf in interfaces:
        intf_path = '/net/%s' % intf
        paths = [intf_path,
                 intf_path + '/arp',
                 intf_path + '/tcp']
        test_paths(node, paths)


@integration_test.test
def test_pci_nodes(node):
    pci_root = node.get('/pci').jdict()
    domains  = [int(d, 16) for d in pci_root.domains]
    for d in domains:
        domain_path = '/pci/%04X' % d
        domain_root = node.get(domain_path).jdict()
        paths       = [domain_path, domain_path + '/cfg']
        paths      += [domain_path + '/' + dev.slot
                       for dev in domain_root.devices]
        test_paths(node, paths)
