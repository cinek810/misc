def addPool2wlcgGrant(grantPrefix,userPrefix,grantList,poolRange=(1,50)):
	from datetime import date
	rok= str(date.today().year)
	for num in range (poolRange[0],poolRange[1]):
		user=userPrefix+"%03d" % (num)
		try:
			grantList[grantPrefix+rok].append((user,grantPrefix+rok))
		except KeyError:
			grantList[grantPrefix+rok]=list();
			grantList[grantPrefix+rok].append((user,grantPrefix+rok))

def addUser2wlcgGrant(grantPrefix,user,grantList):
	from datetime import date
	rok= str(date.today().year)
	try:
		grantList[grantPrefix+rok].append((user,grantPrefix+rok))
	except KeyError:
		grantList[grantPrefix+rok]=list();
		grantList[grantPrefix+rok].append((user,grantPrefix+rok))


def printGrants(parentGrant,grantList):
	for grant in  grantList:
		print ("Account - "+grant+":Description='"+parentGrant+":"+grant+"':Organization='"+parentGrant+"'" )

	for grant, users in grantList.iteritems():
	#check default grant for user
		print ("Parent - "+grant);
		for user in users:
			defAcct=user[1];
			userName=user[0];
			if defAcct!="":
				print ("User - "+userName+":DefaultAccount='"+defAcct+"'")



