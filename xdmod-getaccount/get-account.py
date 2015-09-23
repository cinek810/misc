#!/usr/bin/python
#########################################################################################
#Author: Marcin Stolarek  (stolarek.marcin@gmail.com,mstol@icm.edu.pl)                  #
#Python script to retrrive accounting information about ICM Supercomputers from xdmod.  #
#########################################################################################

import requests,json,sys
requests.packages.urllib3.disable_warnings()

class XDModConnector:
	
	def __init__(self,user,passwo):
		self.user=user
		self.passwo=passwo

		r = requests.get("https://xdmod.grid.icm.edu.pl/rest/authentication/utilities/login/username=testuser/password=testrest",auth=(self.user,self.passwo),verify=False)


		setc=r.headers['set-cookie']
		setc=setc.split(";")
		self.cookies=dict()
		for cookie in setc:
			(name,value)=cookie.split('=')
			self.cookies[name]=value

		jr=r.json()
		self.token=jr["results"]["token"]


	def close(self):
		r = requests.get("https://xdmod.grid.icm.edu.pl/rest/authentication/utilities/logout",auth=(user,passwo),verify=False,cookies=self.cookies)
	
	def getUsageOn(self,resourceName,dim,start,end):
		r = requests.get('https://xdmod.grid.icm.edu.pl/rest/datawarehouse/explorer/dataset/fact='+resourceName+'/dimension='+dim+'/realms/start_date='+start+'/end_date='+end+'/querygroup=tg_usage/format=json/text?token='+self.token, auth=(user,passwo), verify=False,cookies=self.cookies)
		try:
			usage=r.json()
			userUsage=json.loads( usage[0]["rows"])
		except:
			print r.text
			sys.exit(2)

		return userUsage
	

if __name__ == "__main__":
	import argparse
	
	from time import gmtime, strftime
	today=strftime("%Y-%m-%d",gmtime())
		

	import getpass
	user=getpass.getuser()

	parser = argparse.ArgumentParser()
	parser.add_argument('entity',nargs='+',help="Entity for which accounting data should be displayed - can be user, or account/group. Check -g option.")
#	parser.add_argument('-d', dest='debug', action='store_true',help="Run in debug mode")
	parser.add_argument('-c','--cost',dest="display_cost",action="store_true",help="Display estimated overal cost for computations")
	parser.add_argument('-g','--group',dest="group", action="store_true", help="If specified entity is interpreted as account/group rather than user name.")
	parser.add_argument('-D','--bindas',dest="user", help="The user to bind as, as default will try to bind as currently loged user ("+user+").")
	parser.add_argument('-S',"--start_time",dest="start_time",help="The starting date for wich data is requested. In format YYYY-MM-DD. Default is 2013-12-01.")
	parser.add_argument('-E',"--end_time",dest="end_time",help="The ending date for wich data is requested. In format YYYY-MM-DD. Default is today ("+today+").")
	parser.add_argument('-v',"--version",dest="version",action="store_true",help="Display version information")
	args = vars(parser.parse_args())

	if args['user'] is not  None:
		user=args['user']

	passwo=getpass.getpass("LDAP password for "+user+":" )

	if args["version"]==True:
		printf("get-account.py version 1.0. Author: Marcin Stolarek, current sources on https://github.com/cinek810/misc/tree/master/xdmod-getaccount")

	if args["group"]:
		dim="pi"
	else:
		dim="username" 

	if args["start_time"]:
		start_time=args["start_time"]
	else:
		start_time="2013-12-01"

	if args["end_time"]:
		end_time=args["end_time"]
	else:
		end_time=today

	xdmod=XDModConnector(user,passwo)

	for ent in args['entity']:
		print "For "+ent+" usage on:"
		for computer in [ "hydra", "topola", "boreasz", "nostromo", "halo2" ]:
			try:
				print "\t"+computer+" "+str( xdmod.getUsageOn("total_"+computer+"_time",dim,start_time,end_time)[ent][1])+" computing hours"
			except KeyError:
				print "No accounting data for such user"
				sys.exit(2)

		if args["display_cost"]:
			print "Estimated cost:"+str(xdmod.getUsageOn("total_norm_cost",dim,start_time,end_time)[ent][1])+" PLN"
	


	xdmod.close()
