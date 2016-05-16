#!/usr/bin/python

import os,sys,getopt
from lxml import etree

PATH_XML_FILE = "/etc/pdns/loadbalance.xml"

def usage() :
	print sys.argv[0]+' [options] [arguments]'
	print 'options:'
	print '        -i | --init             #initialize and create the XML file'
	print '        -s | --status           #display the content of the XML file'
	print '        -a | --add              #add an element value into the XML file'
	print '        -d | --delete           #delete the element value from the XMl file'
	print '        -u | --update           #update the element value into the XML file'
	print '        -h | --help             #get help info'
	print 'arguments:'
	print '          receive_port          #'
	print '          domainname            #the node that descripes some information for domainname'
	print '          refresh               #the child of domainname,the interval to refresh'
	print '          retry                 #the child of domainname,the interval to retry'
	print '          expiry                #the child of domainname,the expiry time'
	print '          ttl                   #the child of domainname,the time to live'
	print '          serial                #the child of domainname,serial'
	print '          policy                #the child of domainname,policy'
	print '          address               #the attribute of addresspool,address into addresspool'
	print '          weight                #the attribute of addresspool,the weight of address'
	print 'example: '+sys.argv[0]+' --update receive_port=1662'
	print '         '+sys.argv[0]+" --add domainname=www.test.com address='192.168.1.1' weight = 1"

	print 'Notes:'
	print '      when add an address and weight or update refresh .etc,must specify domainname\n'

def xml_exist():
	global PATH_XML_FILE
	if os.path.exists(PATH_XML_FILE):
		return True
	else:
		return False

def init_xml():
	global PATH_XML_FILE
	loadbalance		= etree.Element("loadbalanceconf")
	recv_port		= etree.Element("port",receiver="1662")
	loadbalance.append(recv_port)
	etree.ElementTree(loadbalance).write(PATH_XML_FILE,pretty_print=True,encoding='UTF-8')
	return

def print_xml():
	global PATH_XML_FILE
	parser = etree.XMLParser(remove_blank_text=True)
	tree = etree.parse(PATH_XML_FILE,parser)
	print etree.tostring(tree,pretty_print=True)
	return

def add_domainname(value) :
	global PATH_XML_FILE
	parser = etree.XMLParser(remove_blank_text=True)
	tree = etree.parse(PATH_XML_FILE,parser)

	flags_serial = False
	arg_refresh = None
	arg_retry = None
	arg_expiry = None
	arg_ttl = None
	arg_serial = None
	arg_policy = None
	arg_address = []
	arg_weight = []

	for args in sys.argv[1:] :
		if args.find("refresh=") == 0:
			arg_refresh = args.split('=')[1]
		if args.find("retry=") == 0:
			arg_retry = args.split('=')[1]
		if args.find("expiry=") == 0:
			arg_expiry = args.split('=')[1]
		if args.find("ttl=") == 0:
			arg_ttl = args.split('=')[1]
		if args.find("serial=") == 0:
			arg_serial = args.split('=')[1]
			flags_serial = True
		if args.find("policy=") == 0:
			arg_policy = args.split('=')[1]
			if arg_policy != 'round-robin' and arg_policy != 'link':
				print "the policy must be one of 'round-robin' and 'link' "
				sys.exit(3)
		if args.find("address=") == 0:
			arg_address.append(args.split('=')[1])
		if args.find("weight=") == 0:
			arg_weight.append(args.split('=')[1])
	
	if not flags_serial:
		print 'must specify an serial when add a domainname'
		sys.exit(3)

	if len(arg_address) != len(arg_weight) :
		print 'invalid arguments,the num of address not equal to num of weight'
		sys.exit(3)

	loadbalance 	= tree.getroot()
	domainname		= etree.Element("domainname",name=value)
	refresh 		= etree.Element("time",value="10",type="refresh")
	retry			= etree.Element("time",value="10",type="retry")
	expiry			= etree.Element("time",value="10",type="expiry")
	ttl				= etree.Element("time",value="10",type="ttl")
	serial			= etree.Element("serial",value=arg_serial)
	policy			= etree.Element("policy",value="round-robin")

	address = []
	for index,iterm in enumerate(arg_address):
		address.append(etree.Element("addresspool",address=iterm,weight=arg_weight[index]))
	if arg_refresh :
		refresh.attrib['value'] = arg_refresh
	if arg_retry :
		retry.attrib['value'] = arg_retry
	if arg_expiry:
		expiry.attrib['value'] = arg_expiry
	if arg_ttl :
		ttl.attrib['value'] = arg_ttl
	if arg_policy:
		policy.attrib['value'] = arg_policy
	
	domainname.append(refresh)
	domainname.append(retry)
	domainname.append(expiry)
	domainname.append(ttl)
	domainname.append(serial)
	domainname.append(policy)
	for add in address:
		domainname.append(add)
	loadbalance.append(domainname)
	tree.write(PATH_XML_FILE,pretty_print=True,encoding='UTF-8')

def add_address(value):
	global PATH_XML_FILE
	parser = etree.XMLParser(remove_blank_text=True)
	tree = etree.parse(PATH_XML_FILE,parser)
	loadbalance = tree.getroot()
	domain = tree.xpath("domainname[@name='"+value+"']")

	arg_address = []
	arg_weight = []
	for args in sys.argv[1:] :
		if args.find("address=") == 0:
			arg_address.append(args)
		if args.find("weight=") == 0:
			arg_weight.append(args)

	if len(arg_address) != len(arg_weight) :
		print 'invalid arguments,the num of address not equal to num of weight'
		sys.exit(3)

	address = []
	for index,iterm in enumerate(arg_address):
		address.append(etree.Element("addresspool",address=iterm.split('=')[1],
			weight=arg_weight[index].split('=')[1]))
	for add in address:
		domain[0].append(add)
	tree.write(PATH_XML_FILE,pretty_print=True,encoding='UTF-8')

def add_element_xml(value):
	global PATH_XML_FILE
	value = value.split('=')
	parser = etree.XMLParser(remove_blank_text=True)
	tree = etree.parse(PATH_XML_FILE,parser)
	
	if value[0] != 'domainname' :
		print 'must specify domainname when add an element (domainname or address)'
		print sys.argv[0]+' -h for help'
		sys.exit(3)

	domain = tree.xpath("domainname[@name='"+value[1]+"']")
	if len(domain) == 0 :
		add_domainname(value[1])
	elif len(domain) == 1 :
		add_address(value[1])
	else :
		print 'there are two equal domainnames in the XML file'
		sys.exit(3)

def delete_domain(value):
	global PATH_XML_FILE
	parser = etree.XMLParser(remove_blank_text=True)
	tree = etree.parse(PATH_XML_FILE,parser)
	domain = tree.xpath("domainname[@name='"+value+"']")
	loadbalance = tree.getroot()
	for do in domain:
		loadbalance.remove(do)
	tree.write(PATH_XML_FILE,pretty_print=True,encoding='UTF-8')

def delete_address(value,arg_address):
	global PATH_XML_FILE
	parser = etree.XMLParser(remove_blank_text=True)
	tree = etree.parse(PATH_XML_FILE,parser)
	domain = tree.xpath("domainname[@name='"+value+"']")

	for do in domain:
		for child in do:
			for arg in arg_address :
				if child.attrib.get('address') == arg :
					do.remove(child)
	tree.write(PATH_XML_FILE,pretty_print=True,encoding='UTF-8')

def delete_element_xml(value):
	global PATH_XML_FILE
	value = value.split('=')
	parser = etree.XMLParser(remove_blank_text=True)
	tree = etree.parse(PATH_XML_FILE,parser)

	if value[0] != 'domainname' :
		print 'must specify domainname when add an element (domainname or address)'
		print sys.argv[0]+' -h for help'
		sys.exit(3)
	domain = tree.xpath("domainname[@name='"+value[1]+"']")
	arg_address = []
	if len(domain) == 0:
		print 'the domain want to be deleted is not exist'
		sys.exit(3)
	elif len(domain) >= 1:
		for args in sys.argv[1:] :
			if args.find("address=") == 0:
				arg_address.append(args.split('=')[1])
	if arg_address :
		delete_address(value[1],arg_address)
	else :
		delete_domain(value[1])

def update_receive_port(value):
	global PATH_XML_FILE
	parser = etree.XMLParser(remove_blank_text=True)
	tree = etree.parse(PATH_XML_FILE,parser)
	loadbalance = tree.getroot()
	for child in loadbalance :
		if child.tag == 'port':
			child.attrib['receiver'] = value
	tree.write(PATH_XML_FILE,pretty_print=True,encoding='UTF-8')

def update_domainname(value):
	global PATH_XML_FILE
	parser = etree.XMLParser(remove_blank_text=True)
	tree = etree.parse(PATH_XML_FILE,parser)
	domain = tree.xpath("domainname[@name='"+value+"']")
	if len(domain) == 0:
		print 'the specify domainname is not exist'
		sys.exit(3)
	
	flags_serial = False
	arg_refresh = None
	arg_retry = None
	arg_expiry = None
	arg_ttl = None
	arg_serial = None
	arg_policy = None
	arg_address = []
	arg_weight = []

	for args in sys.argv[1:] :
		if args.find("refresh=") == 0:
			arg_refresh = args.split('=')[1]
		if args.find("retry=") == 0:
			arg_retry = args.split('=')[1]
		if args.find("expiry=") == 0:
			arg_expiry = args.split('=')[1]
		if args.find("ttl=") == 0:
			arg_ttl = args.split('=')[1]
		if args.find("serial=") == 0:
			arg_serial = args.split('=')[1]
		if args.find("policy=") == 0:
			arg_policy = args.split('=')[1]
			if arg_policy != 'round-robin' and arg_policy != 'link':
				print "the policy must be one of 'round-robin' and 'link' "
				sys.exit(3)
		if args.find("address=") == 0:
			arg_address.append(args.split('=')[1])
		if args.find("weight=") == 0:
			arg_weight.append(args.split('=')[1])

	if len(arg_address) != len(arg_weight) :
		print 'invalid arguments,the num of address not equal to num of weight'
		sys.exit(3)

	for do in domain:
		for child in do:
			if arg_refresh and child.attrib.get('type') == 'refresh':
				child.attrib['value'] = arg_refresh
			if arg_retry and child.attrib.get('type') == 'retry':
				child.attrib['value'] = arg_retry
			if arg_expiry and child.attrib.get('type') == 'expiry':
				child.attrib['value'] = arg_expiry
			if arg_ttl and child.attrib.get('type') == 'ttl':
				child.attrib['value'] = arg_ttl
			if arg_serial and child.tag == 'serial':
				child.attrib['value'] = arg_serial
			if arg_policy and child.tag == 'policy':
				child.attrib['value'] = arg_policy
			for index,iterm in enumerate(arg_address):
				if iterm == child.attrib.get('address') :
					child.attrib['weight'] = arg_weight[index]
	tree.write(PATH_XML_FILE,pretty_print=True,encoding='UTF-8')

def update_element_xml(value):
	value = value.split('=')
	if value[0] == 'receive_port' :
		update_receive_port(value[1])
	elif value[0] == 'domainname' :
		update_domainname(value[1])
	else :
		print 'invalid arguments,Please specify receive_port or domainname arguments'
		sys.exit(3)


def parse_args():
	try :
		opts,args = getopt.getopt(sys.argv[1:],'hisa:d:u:',
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
		print 'The xml file is not exist'
		print sys.argv[0]+' -h for help'
		sys.exit(3)
	return

def main() :
	parse_args()

if __name__ == '__main__' :
	main()
