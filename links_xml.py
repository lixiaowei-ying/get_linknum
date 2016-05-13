#!/usr/bin/python

import os,sys,getopt
from lxml import etree

PATH_XML_FILE = "/root/links.xml"

def usage():
	print sys.argv[0] + ' [options] [argments]'
	print 'options:'
	print '        -i | --init             #initialize and create the XML file'
	print '        -s | --status           #display the content of the XML file'
	print '        -a | --add              #add an element value into the XML file'
	print '        -d | --delete           #delete the element value from the XML file'
	print '        -u | --update           #update the element value into the XML file'
	print '        -h | --help             #get help'
	print 'arguments:'
	print '         collect_interval       #collect links interval in seconds,default 10'
	print '         send_interval          #send links interval in seconds,default 10'
	print '         dst_ip                 #the destination IP'
	print '         dst_port               #the destination port,default 1662'
	print '         community              #the SNMP community,default public'
	print '         version                #the SNMP version,the value must be 2'
	print '         cifs_port              #protocol CIFS port,default 445,could be added'
	print '         nfs_port               #protocol NFS port,default 2049,could be added'
	print 'example: '+sys.argv[0]+' --delete cifs_port=123'
	print '         '+sys.argv[0]+' --update dst_ip=192.168.1.1'
	print 'Notes:'
	print "      The delete and add options only fit cifs_port and nfs_port arguments"
	print "      The update option couldn't fit cifs_port and nfs_port arguments\n"

def xml_exist():
	global PATH_XML_FILE
	if os.path.exists(PATH_XML_FILE):
		return True
	else:
		return False

def init_xml():
	global PATH_XML_FILE
	numconn 	= etree.Element("Numconn")
	inter_coll 	= etree.Element("Interval",name="Collect")
	inter_send	= etree.Element("Interval",name="Send")
	snmp 		= etree.Element("Snmp")
	dst_ip		= etree.Element("Dst_ip")
	dst_port	= etree.Element("Dst_port")
	community	= etree.Element("Community")
	version		= etree.Element("Version")
	proto_cifs	= etree.Element("Protocol",name="CIFS")
	proto_nfs	= etree.Element("Protocol",name="NFS")
	cifs_port	= etree.Element("Port")
	nfs_port	= etree.Element("Port")

	inter_coll.text = '10'
	inter_send.text = '60'
	dst_ip.text = '192.168.1.1'
	dst_port.text = '1662'
	community.text = 'public'
	version.text = '2'
	cifs_port.text = '445'
	nfs_port.text = '2049'

	numconn.append(inter_coll)
	numconn.append(inter_send)
	numconn.append(snmp)
	numconn.append(proto_cifs)
	numconn.append(proto_nfs)

	snmp.append(dst_ip)
	snmp.append(dst_port)
	snmp.append(community)
	snmp.append(version)

	proto_cifs.append(cifs_port)
	proto_nfs.append(nfs_port)

	etree.ElementTree(numconn).write(PATH_XML_FILE,pretty_print=True,encoding='UTF-8')
	return

def print_xml():
	global PATH_XML_FILE
	parser = etree.XMLParser(remove_blank_text=True)
	tree = etree.parse(PATH_XML_FILE,parser)
	print etree.tostring(tree,pretty_print=True)
	return

def add_element_xml(value):
	global PATH_XML_FILE
	value = value.split('=')
	parser = etree.XMLParser(remove_blank_text=True)
	tree = etree.parse(PATH_XML_FILE,parser)
	
	if value[0] == 'cifs_port' :
		protocol = tree.xpath("Protocol[@name='CIFS']")
		cifs_port = etree.Element("Port")
		cifs_port.text = value[1]
		protocol[0].append(cifs_port)
		tree.write(PATH_XML_FILE,pretty_print=True,encoding='UTF-8')
	elif value[0] == 'nfs_port' :
		protocol = tree.xpath("Protocol[@name='NFS']")
		nfs_port = etree.Element("Port")
		nfs_port.text = value[1]
		protocol[0].append(nfs_port)
		tree.write(PATH_XML_FILE,pretty_print=True,encoding='UTF-8')
	else:
		print 'The arguments '+value[0]+' is invalid'
	return

def delete_element_xml(value):
	global PATH_XML_FILE
	value = value.split('=')
	parser = etree.XMLParser(remove_blank_text=True)
	tree = etree.parse(PATH_XML_FILE,parser)
	
	if value[0] == 'cifs_port' :
		protocol = tree.xpath("Protocol[@name='CIFS']")
	elif value[0] == 'nfs_port' :
		protocol = tree.xpath("Protocol[@name='NFS']")
	else:
		print 'The arguments '+value[0]+' is invalid'
		return
	for child in protocol[0] :
		if child.text == value[1]:
			protocol[0].remove(child)
	tree.write(PATH_XML_FILE,pretty_print=True,encoding='UTF-8')
	return

def update_element_xml(value):
	global PATH_XML_FILE
	value = value.split('=')
	parser = etree.XMLParser(remove_blank_text=True)
	tree = etree.parse(PATH_XML_FILE,parser)
	
	if value[0] == 'collect_interval' :
		inter = tree.xpath("Interval[@name='Collect']")
	elif value[0] == 'send_interval' :
		inter = tree.xpath("Interval[@name='Send']")
	elif value[0] == 'dst_ip' :
		inter = tree.xpath("//Dst_ip")
	elif value[0] == 'dst_port' :
		inter = tree.xpath("//Dst_port")
	elif value[0] == 'community' :
		inter = tree.xpath("//Community")
	elif value[0] == 'version' :
		inter = tree.xpath("//Version")
	else :
		print 'The arguments '+value[0]+' is invalid'
		return
	inter[0].text = value[1]
	tree.write(PATH_XML_FILE,pretty_print=True,encoding='UTF-8')
	return

def parse_args():
	try:
		opts, args = getopt.getopt(sys.argv[1:],'hisa:d:u:',
				['help','init','status','add=','delete=','update='])
	except getopt.GetoptError,err:
		print str(err)
		print sys.argv[0]+' -h for help'
		sys.exit(2)
	
	flags_init = False
	for op,value in opts :
		if op in ('-h','--help'):
			usage()
			sys.exit(1)
		elif op in ('-i','--init'):
			init_xml()
			flags_init = True
		elif op in ('-s','--status'):
			print_xml()
		elif op in ('-a','--add'):
			add_element_xml(value)
		elif op in ('-d','--delete'):
			delete_element_xml(value)
		elif op in ('-u','--update'):
			update_element_xml(value)
		else:
			print 'unhandled options'
			print sys.argv[0]+' -h for help'
			sys.exit(3)

	if not xml_exist() and not flags_init:
		print "The XML file is not exist"
		print sys.argv[0]+' -h for help'
		sys.exit(3)

def main():
	parse_args()

if __name__ == '__main__' :
	main()
