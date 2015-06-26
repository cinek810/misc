#!/usr/bin/python

import pexpect as pe
import re
import sys
import ldap


class ldapGetter:
	
	def __init__(self,address):
		self.ldapHandler=ldap.open(address);
	def  getUsers(self,dn):
		queryresult=self.ldapHandler.search_s(dn,ldap.SCOPE_SUBTREE,filterstr='(objectclass=posixaccount)',attrlist=['uid','uidNumber'])
		users=dict()
		for entry in queryresult:
			users[entry[1]["uidNumber"][0]]=([entry[1]["uid"][0],entry[1]["uidNumber"][0],entry[1]["uid"][0]+"@nfs.icm.edu.pl"])
		return users

	def  getGroups(self,dn):
		queryresult=self.ldapHandler.search_s(dn,ldap.SCOPE_SUBTREE,filterstr='(objectclass=posixGroup)',attrlist=['gidNumber','cn'])
		groups=dict()
		for entry in queryresult:
			groups[entry[1]["gidNumber"][0]]=([entry[1]["cn"][0],entry[1]["gidNumber"][0],entry[1]["cn"][0]+"@nfs.icm.edu.pl"])
		return groups


			
		
	

class hnasSmu:
	""" Class representing hnasSmu"""
	cmd = "ssh admin@hnas-smu.m"

	

	def __init__(self,cmd):
		self.cmd=cmd;
	
		self.hnas=pe.spawn(cmd)
	#	self.hnas.logfile=sys.stdout
		self.hnas.expect("Server: ")
		self.hnas.sendline("1")
		self.hnas.expect(":\$ ")
	
	def getUsers(self):
		self.hnas.sendline("user-mappings-list")
		self.hnas.expect(":\$ ")
		output=self.hnas.before
		users=dict()
		for line in output.split('\n'):
			#searchResult=re.search("([a-z\*]+)\s+\(([0-9]+)\)\s+[a-z\*]+\s+\([a-z\*]\)\s+([a-z@]+)",line)
			searchResult=re.search("([\._\-a-z0-9\*]+)\s+\(([0-9\*a-z]+)\)\s+[\._\-0-9a-z\*]+\s+\([\._\-0-9a-z\*\)]+\s+([\._\-0-9a-z@\.\*]+)\s+",line)
			if searchResult is not None:
				tmpSearch=re.search("([\._\-a-z0-9]+).*",searchResult.group(1))
				uname=tmpSearch.group(1)
				tmpSearch=re.search("([\._\-a-z0-9]+).*",searchResult.group(2))
				uid=tmpSearch.group(1)
				tmpSearch=re.search("([\._\-a-z0-9@\.]+).*",searchResult.group(3))
				nfsmapping=tmpSearch.group(1)
				users[uid]=[uname,uid,nfsmapping]

		return users

	def getGroups(self):
		self.hnas.sendline("group-mappings-list")
		self.hnas.expect(":\$ ")
		output=self.hnas.before
		groups=dict()
		for line in output.split('\n'):
			#searchResult=re.search("([a-z\*]+)\s+\(([0-9]+)\)\s+[a-z\*]+\s+\([a-z\*]\)\s+([a-z@]+)",line)
			searchResult=re.search("([\._\-a-zA-Z0-9\*\-]+)\s+\(([0-9\*a-z]+)\)\s+[\._\-0-9a-z\*]+\s+\([\._\-0-9a-z\*\)]+\s+([\._\-0-9a-zA-Z@\.\*]+)\s+",line)
			if searchResult is not None:
				tmpSearch=re.search("([\._\-a-z0-9A-Z]+).*",searchResult.group(1))
				gname=tmpSearch.group(1)
				tmpSearch=re.search("([\._\-a-z0-9A-Z]+).*",searchResult.group(2))
				gid=tmpSearch.group(1)
				tmpSearch=re.search("([\._\-A-Za-z0-9@\.]+).*",searchResult.group(3))
				nfsmapping=tmpSearch.group(1)
				groups[gid]=[gname,gid,nfsmapping]

		print groups
		return groups

	def addGroup(self,group):
		self.hnas.sendline("group-mappings-add --unix-name "+group[0]+" --unix-id "+group[1]+" --nfsv4-name "+group[2])
		self.hnas.expect(":\$ ")
		result=re.search("(.*error.*)",self.hnas.before,re.IGNORECASE)
		if result!=None:
			sys.stderr.write(result.group(1)+"\n")



	def deleteUser(self,user):
		if (re.search("unknown",user[1]) is None):
			self.hnas.sendline("user-mappings-delete --unix-id "+user[1])
		elif (re.search("unknown",user[0]) is None):
			self.hnas.sendline("user-mappings-delete --unix-name "+user[0])
		elif (re.search("unknown",user[2]) is None):
			self.hnas.sendline("user-mappings-delete --nfsv4-name "+user[2])
		else:
			sys.stderr.write( "Cannot delete user")


		self.hnas.expect(":\$ ")

	def deleteGroup(self,group):
		if (re.search("unknown",group[1]) is None):
			self.hnas.sendline("group-mappings-delete --unix-id "+group[1])
		elif (re.search("unknown",group[0]) is None):
			self.hnas.sendline("group-mappings-delete --unix-name "+group[0])
		elif (re.search("unknown",group[2]) is None):
			self.hnas.sendline("group-mappings-delete --nfsv4-name "+group[2])
		else:
			sys.stderr.write( "Cannot delete group")

		self.hnas.expect(":\$ ")
	
	def setGroupNames(self,group):
		if (re.search("[0-9]",group[1])!=None):
			self.hnas.sendline("group-mappings-modify --unix-id "+group[1]+" --set-unix-id "+group[1]+"  --set-unix-name "+group[0]+" --set-nfsv4-name "+group[2])
		else:
			sys.stderr.write("Cannot set names - bad unix id")
	


	def addUser(self,user):
		self.hnas.sendline("user-mappings-add --unix-name "+user[0]+" --unix-id "+user[1]+" --nfsv4-name "+user[2])
		self.hnas.expect(":\$ ")
		result=re.search("(.*error.*)",self.hnas.before,re.IGNORECASE)
		if result!=None:
			sys.stderr.write(result.group(1)+"\n")

	def setUserNames(self,user):
		if (re.search("[0-9]",user[1])!=None):
			self.hnas.sendline("user-mappings-modify --unix-id "+user[1]+" --set-unix-id "+user[1]+"  --set-unix-name "+user[0]+" --set-nfsv4-name "+user[2])
		else:
			sys.stderr.write("Cannot set names - bad unix id")
		
		


if __name__ == "__main__":
	hnas=hnasSmu("ssh admin@hnas-smu.m")
	
	print("Getting users from HNAS")
	usersOnHnas=hnas.getUsers();


	print("Getting groups from HNAS")
	groupsOnHNAS=hnas.getGroups();

	ldapUsers=ldapGetter("192.168.69.110")

	print("Getting hydra users from LDAP")
	hydraUsers=ldapUsers.getUsers("o=hydra");
	hydraGroups=ldapUsers.getGroups("o=hydra");
	allUsers=hydraUsers
	allGroups=hydraGroups

	print ("Getting icm users from LDAP")
	icmUsers=ldapUsers.getUsers("dc=icm,dc=edu,dc=pl")
	icmGroups=ldapUsers.getGroups("dc=icm,dc=edu,dc=pl")
	allUsers.update(icmUsers)
	allGroups.update(icmGroups)


	print ("Getting plgrid users from LDAP")
	plgridUsers=ldapUsers.getUsers("dc=icm,dc=plgrid,dc=pl")
	plgridGroups=ldapUsers.getGroups("dc=icm,dc=plgrid,dc=pl")
	allUsers.update(plgridUsers)
	allGroups.update(plgridGroups)


#	print("Deleting all users")
#	for user in users:
#		hnas.deleteUser(user)



        for (uid,user) in usersOnHnas.items():
        	try:
        		if  allUsers[uid][0]!=usersOnHnas[uid][0] or allUsers[uid][2]!=usersOnHnas[uid][2]:
        			sys.stderr.write("Updating user "+str(user)+" to "+str(allUsers[uid]))
        			hnas.setUserNames(allUsers[uid])
        		else:
        			sys.stderr.write("===User consistent in LDAP and on HNAS:"+ str(allUsers[uid])+"\n")
				#Remove user from allUsers - the list will containt only new users after whole loop
        			del allUsers[uid]
				#Remove users from userOnHnas - the list will contain only users that are configured on hnas but they don't exist on LDAP
        			del usersOnHnas[uid]
        		
        	except KeyError:
        		if user[0]!="unknown":
        			hnas.deleteUser(user)		
        			del usersOnHnas[uid]
        			sys.stderr.write( "Deleting user from HNAS.User not found in LDAP:"+str(user)+"\n")
        		else:
        			sys.stderr.write( "===Probably local user, you should add it manually: "+str(user)+"\n")	
        		
        

        print "Adding new users from LDAP"	
        for (uid,user) in allUsers.items():
        	sys.stderr.write("Adding user:"+str(user)+" (Error may mean that you have this unix name with second uid)\n")
        	hnas.addUser(user)

        
        for (uid,user) in usersOnHnas.items():
        	sys.stderr.write("Not considered on Hnas. Mayby this is locall user and should be modiffied manually:"+str(user)+" ?\n")

        
	##DELETING ALL GROUPS ON HNAS
	#for gid,group in  (hnas.getGroups()).iteritems():
	#	hnas.deleteGroup(group)

        for (gid,group) in groupsOnHNAS.items():
        	try:
        		if  allGroups[gid][0]!=groupsOnHNAS[gid][0] or allGroups[gid][2]!=groupsOnHNAS[gid][2]:
        			sys.stderr.write("Updating group "+str(group)+" to "+str(allGroups[gid])+"\n")
        			hnas.setGroupNames(allGroups[gid])
        		else:
        			sys.stderr.write("===Group consistent in LDAP and on HNAS:"+ str(allGroups[gid])+"\n")
				#The same logic as with users:
        			del allGroups[gid]
        			del groupsOnHNAS[gid]
        		
        	except KeyError:
        		if group[0]!="unknown":
        			hnas.deleteGroup(group)		
        			del groupsOnHNAS[gid]
        			sys.stderr.write( "Deleting group from HNAS.Group not found in LDAP:"+str(group)+"\n")
        		else:
        			sys.stderr.write( "===Probably local group, you should add it manually: "+str(group)+"\n")	
        		
        

        print "Adding new groups from LDAP"	
        for (gid,group) in allGroups.items():
        	sys.stderr.write("Adding group:"+str(group)+" (Error may mean that you have this unix name with second uid)\n")
        	hnas.addGroup(group)

        
        for (gid,group) in groupsOnHNAS.items():
        	sys.stderr.write("Not considered on Hnas. Mayby this is locall group and should be modiffied manually:"+str(group)+" ?\n")

