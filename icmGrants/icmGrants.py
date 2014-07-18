#!/usr/bin/python

import ldap
import re
import tempfile
from libIcmGrants import *



#polaczenie do ldapa
l = ldap.open("192.168.69.110")


#WSTEP DO PLIKU
print ("Cluster - 'hydra':Fairshare=1:QOS='htc,normal'")
#print ("Parent - root" )
print ("User - root:DefaultAccount='root':AdminLevel='Administrator':Fairshare=1" )
#print ("Account - icm:Description='icm':Organization='icm':Fairshare=1" )
#print ("Account - plgrid:Description='plgrid':Organization='plgrid':Fairshare=1" )
#print ("Account - wlcg:Description='wlcg':Organization='wlcg':Fairshare=1" )




#GENERACJA GRANTOW ICM
l_query_uids=l.search_s("dc=icm,dc=edu,dc=pl",ldap.SCOPE_SUBTREE,filterstr='(objectClass=posixAccount)',attrlist=['uid'])

grant_dict=dict();





for dn,uid in l_query_uids:
	user_uid=uid['uid'][0];
	l_query_groups=l.search_s("dc=icm,dc=edu,dc=pl",ldap.SCOPE_SUBTREE,filterstr='(&(objectClass=posixGroup)(memberUid='+user_uid+'))',attrlist=['cn']);
	for dn,group_name in l_query_groups:
		grant=group_name['cn'][0]
		if ( (re.search("[GD][0-9][0-9]-[0-9]+",grant) != None) or (re.search("app-installers",grant) != None ) or  (re.search("icm-meteo",grant) != None )):
			try:
				grant_dict[grant].append((user_uid,grant));
			except KeyError:
				grant_dict[grant]=list()
				grant_dict[grant].append((user_uid,grant));


printGrants("icm",grant_dict);

#GENERACJA GRANTOW WLCG
grant_plg=dict()

#OPS&DTEAM
addPool2wlcgGrant('voops','ops',grant_plg,(1,50));
addUser2wlcgGrant('voops','opssgm',grant_plg);
addPool2wlcgGrant('voops','dteam',grant_plg,(1,50));

#LHCB
addPool2wlcgGrant('volhcb','lhcb',grant_plg,(1,199));
addPool2wlcgGrant('volhcb','pltlhcb',grant_plg,(1,199));
addUser2wlcgGrant('volhcb','lhcbsgm',grant_plg)
addUser2wlcgGrant('volhcb','lhcbprd',grant_plg)

#CMS
addPool2wlcgGrant('vocms','cms',grant_plg,(1,199));
addPool2wlcgGrant('vocms','pltcms',grant_plg,(1,99));
addUser2wlcgGrant('vocms','cmsprd',grant_plg);
addUser2wlcgGrant('vocms','cmssgm',grant_plg);



#Zbieranie informacji do grantow plgrid
plgridDN="dc=icm,dc=plgrid,dc=pl";
l_query_uids=l.search_s(plgridDN,ldap.SCOPE_SUBTREE,filterstr='(objectClass=plgridOrgPerson)',attrlist=['uid','plgridPrivateGrantID','plgridDefaultGrantID'])



import sys

for dn,attrs in l_query_uids:
	user_uid=attrs['uid'][0];

	try:
		priv_grant=attrs['plgridPrivateGrantID'][0];
	except KeyError:
		sys.stderr.write("User "+user_uid+" doesn't have priv_grant\n");
		priv_grant=0

	try:
		userDefAcct=attrs['plgridDefaultGrantID'][0];
		l_check_grant=l.search_s(plgridDN,ldap.SCOPE_SUBTREE,filterstr='(&(objectClass=posixGroup)(plgridGrantID='+userDefAcct+'))',attrlist=['plgridGrantID'])

		if len(l_check_grant)==0  and userDefAcct!=priv_grant:
			sys.stderr.write("Default account for user:"+user_uid+" with account "+userDefAcct+" doesnt exist. User won't be added without default account\n");
			userDefAcct=""
	except KeyError:
		sys.stderr.write("User doesn't have default grant: "+user_uid+". User won't be added without default account\n");
		userDefAcct="";
	

	if priv_grant!=0:
		grant_plg[priv_grant]=list()
		grant_plg[priv_grant].append((user_uid,userDefAcct))

	l_query_groups=l.search_s(plgridDN,ldap.SCOPE_SUBTREE,filterstr='(&(objectClass=posixGroup)(memberUid='+user_uid+'))',attrlist=['plgridGrantID']);

	for dn,group_name in l_query_groups:
		try:
			grant_list=group_name['plgridGrantID'];
			
			for grant in grant_list:
				try:
					grant_plg[grant].append((user_uid,userDefAcct));
				except KeyError:
					grant_plg[grant]=list()
					grant_plg[grant].append((user_uid,userDefAcct));
		except KeyError:
			#print "group doesn't have plgridGrantID"
			grant_list=[]
			
		
	

printGrants("plgrid",grant_plg);

