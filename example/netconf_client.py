import dataclasses

from ncclient import manager
from lxml import etree


@dataclasses.dataclass
class ConnectionParams:
    host: str = "127.0.0.1"
    port: int = 8330
    # key_path: str = os.path.expanduser("~/.ssh/id_rsa")
    password: str = "netconf"
    user: str = "root"


def netconf_connection(cp: ConnectionParams):
    return manager.connect(
        host=cp.host,
        port=cp.port,
        username=cp.user,
        hostkey_verify=False,
        # key_filename=cp.key_path,
        password=cp.password
    )


def get_xpath(cp: ConnectionParams, xpath: str):
    with netconf_connection(cp) as nc_conn:
        return nc_conn.get(filter=("xpath", xpath))


def get_subtree(cp: ConnectionParams, subtree: str):
    with netconf_connection(cp) as nc_conn:
        return nc_conn.get(filter=("subtree", subtree))


YANG_LIBRARY_SUBTREE = """
<yang-library xmlns="urn:ietf:params:xml:ns:yang:ietf-yang-library">
    <module-set>
        <module/>
    </module-set>
</yang-library>
"""
#NETCONF_NOTIFICATION_SUBTREE doesn't work
NETCONF_NOTIFICATION_SUBTREE = """
<ietf-notification-sequencing xmlns="urn:ietf:params:xml:ns:yang:ietf-notification-sequencing"/>
"""
INTERFACES_SUBTREE = """
<interfaces xmlns="urn:ietf:params:xml:ns:yang:ietf-interfaces"/>
"""
INTERFACES_XPATH = """
<interfaces xmlns="/ietf-interfaces:interfaces"/>
"""

NETCONF_MONITROING ="""
<netconf-monitoring xmlns="urn:ietf:params:xml:ns:yang:ietf-netconf-monitoring"/>
"""


def pretty_print(xml_node):
    print(etree.tostring(xml_node, pretty_print=True).decode())


def test_get_yanglib() :
    result = get_subtree(ConnectionParams(), YANG_LIBRARY_SUBTREE)
    pretty_print(result.data)


if __name__ == '__main__':
    
    test_get_yanglib()