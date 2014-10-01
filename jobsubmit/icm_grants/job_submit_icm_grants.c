/*****************************************************************************\
 *  job_submit_logging.c - Log job submit request specifications.
 *****************************************************************************
 *  Copyright (C) 2010 Lawrence Livermore National Security.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Morris Jette <jette1@llnl.gov>
 *  CODE-OCEC-09-009. All rights reserved.
 *
 *  This file is part of SLURM, a resource management program.
 *  For details, see <http://slurm.schedmd.com/>.
 *  Please also read the included file: DISCLAIMER.
 *
 *  SLURM is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  In addition, as a special exception, the copyright holders give permission
 *  to link the code of portions of this program with the OpenSSL library under
 *  certain conditions as described in each individual source file, and
 *  distribute linked combinations including the two. You must obey the GNU
 *  General Public License in all respects for all of the code used other than
 *  OpenSSL. If you modify file(s) with this exception, you may extend this
 *  exception to your version of the file(s), but you are not obligated to do
 *  so. If you do not wish to do so, delete this exception statement from your
 *  version.  If you delete this exception statement from all source files in
 *  the program, then also delete it here.
 *
 *  SLURM is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with SLURM; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA.
\*****************************************************************************/

#if HAVE_CONFIG_H
#  include "config.h"
#  if STDC_HEADERS
#    include <string.h>
#  endif
#  if HAVE_SYS_TYPES_H
#    include <sys/types.h>
#  endif /* HAVE_SYS_TYPES_H */
#  if HAVE_UNISTD_H
#    include <unistd.h>
#  endif
#  if HAVE_INTTYPES_H
#    include <inttypes.h>
#  else /* ! HAVE_INTTYPES_H */
#    if HAVE_STDINT_H
#      include <stdint.h>
#    endif
#  endif /* HAVE_INTTYPES_H */
#else /* ! HAVE_CONFIG_H */
#  include <sys/types.h>
#  include <unistd.h>
#  include <stdint.h>
#  include <string.h>
#endif /* HAVE_CONFIG_H */

#include <stdio.h>

#include "slurm/slurm.h"
#include "slurm/slurm_errno.h"

#include "src/common/slurm_xlator.h"
#include "src/slurmctld/slurmctld.h"


//Included for plugin functionalities
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <ctype.h>
#include <regex.h>


/*
 * These variables are required by the generic plugin interface.  If they
 * are not found in the plugin, the plugin loader will ignore it.
 *
 * plugin_name - a string giving a human-readable description of the
 * plugin.  There is no maximum length, but the symbol must refer to
 * a valid string.
 *
 * plugin_type - a string suggesting the type of the plugin or its
 * applicability to a particular form of data or method of data handling.
 * If the low-level plugin API is used, the contents of this string are
 * unimportant and may be anything.  SLURM uses the higher-level plugin
 * interface which requires this string to be of the form
 *
 *	<application>/<method>
 *
 * where <application> is a description of the intended application of
 * the plugin (e.g., "auth" for SLURM authentication) and <method> is a
 * description of how this plugin satisfies that application.  SLURM will
 * only load authentication plugins if the plugin_type string has a prefix
 * of "auth/".
 *
 * plugin_version   - specifies the version number of the plugin.
 * min_plug_version - specifies the minumum version number of incoming
 *                    messages that this plugin can accept
 */
const char plugin_name[]       	= "Job submit icm  accounts";
const char plugin_type[]       	= "job_submit/icm_grants";
const uint32_t plugin_version   = 110;
const uint32_t min_plug_version = 100;



char ** getUserGrantsICM(struct passwd * pw)
{
	int currentGrant=0;
	gid_t *groups=NULL;
	char **userGrants = (char **)xmalloc( sizeof(char *));
	int numberOfGroups=0;
	char ** groupsList;
	int ret,reti;
	regex_t regex;	

	reti = regcomp(&regex,"^[gG][0-9]+-[0-9]+",REG_EXTENDED);
	if (reti)
	{
		error("[ICM-GRANTS]Failed to compile regexp");
	}
	debug2("[ICM-GRANTS] Regexp compilled");

	//Allocate for one group	
	do
	{
		numberOfGroups++;
		groups=realloc(groups,numberOfGroups * sizeof(gid_t));
		ret=getgrouplist(pw->pw_name,pw->pw_gid,groups,&numberOfGroups);
		debug2("[ICM-GRANTS]groups:%d",numberOfGroups);
	}
	while(ret==-1);

 	int j=0,k=0;

	groupsList=malloc((numberOfGroups+1) * sizeof(char*));

	struct group *gr=NULL;	
	debug("[ICM-GRANTS]Number of groups = %d",numberOfGroups);
        for (j = 0; j < numberOfGroups; j++) 
	{
            gr = getgrgid(groups[j]);
	    if(groups[j]==NULL)continue;
	    reti=regexec(&regex,gr->gr_name,0,NULL,0);
	    groupsList[j]=NULL;
            if(!reti)
	    {
		    if (gr != NULL)
			groupsList[k]=malloc((strlen(gr->gr_name)+1)*sizeof(char));
			strcpy(groupsList[k],gr->gr_name);

			int i=0;
			do{ groupsList[k][i]=tolower(groupsList[k][i]); }while(groupsList[k][++i]!=NULL);
			debug2("[ICM-GRANTS]adding:%s to users grant list",groupsList[k]);
			k++;
	    }
	    else
		debug2("[ICM-GRANTS]group:%s doesn't match regexp",gr->gr_name);
        }

	groupsList[k]=malloc(2* sizeof(char));
	groupsList[k]=NULL;
	regfree(&regex);

	for (j=0;groupsList[j]!=NULL;j++)
		debug2("groupsList[%d]=%s",j,groupsList[j]);	
	
	return groupsList;
	
}


int init (void)
{
	info("[ICM-GRANTS]:: initialization of '%s'", plugin_name);
	info("[ICM-GRANTS]:: plugin '%s' is ready", plugin_name);
	return SLURM_SUCCESS;
}


int genGrantList(char* list,char** groupsList)
{
	int j=0;
	list[0]=NULL;
	for (j=0;groupsList[j]!=NULL;j++)
	{
		if(j==0) strcpy(list,"You can use one of this accounts:");
                strcat(list,groupsList[j]);
		strcat(list," ");

	}

	if(j==0)
		strcpy(list,"You doesn't have account in ICM, please contact pomoc@icm.edu.pl");

	strcat(list,".");

	return 0;
}



extern int job_submit(struct job_descriptor *job_desc, uint32_t submit_uid,
		      char **err_msg)
{
	int i=0;
	struct passwd * pw;
	char** userGrants;
	char grantList[256];

	if (job_desc->partition!=NULL &&  strncmp(job_desc->partition, "hydra", 5)== 0 && submit_uid>500)
	{//Check if user has access to this ICM grant
		pw=getpwuid (submit_uid);
		if (!pw)
		{
			error ("[ICM-GRANTS]: Error looking up uid=%d in passwd",submit_uid);
			return SLURM_ERROR;
		}
		debug("[ICM-GRANTS]: New job from uid:%d, user:%s", submit_uid, pw->pw_name);

		if (submit_uid!=0)
		{
			debug2("[ICM-GRANTS]getUserGrantsICM starting");
			userGrants=getUserGrantsICM(pw);
			debug2("[ICM-GRANTS]getUserGrantsICM finished successfully");

			if(job_desc->account == NULL)
			{	
				char userInfo[400]="You have to specify account in which you are running this job. Use -A option to the sbatch/srun command.";
				debug2("[ICM-GRANTS]Calling genGrantList");
				genGrantList(grantList,userGrants);
				debug2("[ICM-GRANTS]Finished successfully genGrantList");
				strcat(userInfo,grantList);
				*err_msg=xstrdup(userInfo);
				info("[ICM-GRANTS]Forbiddent job from uid: %d, user:%s without acconut.",submit_uid,pw->pw_name);
				return ESLURM_INVALID_ACCOUNT;
			}

			for(i=0;userGrants[i]!=NULL;i++)
			{
				debug2("[ICM-GRANTS]User grant is:%s comparing with %s",job_desc->account,userGrants[i]);
				if (strcmp(userGrants[i],job_desc->account)==0)
				{
					debug2("[ICM-GRANTS]Grant %s allowed",job_desc->account);
					return SLURM_SUCCESS;
				}
			}
			char userInfo[400]="You are not authorized to use specified account. Check if you specified correct grant with -A option.";
			debug2("[ICM-GRANTS]Calling genGrantList");
			genGrantList(grantList,userGrants);
			debug2("[ICM-GRANTS]Finished successfully genGrantList");
			strcat(userInfo,grantList);
			*err_msg=xstrdup(userInfo);

			info("[ICM-GRANT]Forbiddent job from uid:%d, user:%s with acconut:%s",submit_uid,pw->pw_name,job_desc->account);
			return ESLURM_INVALID_ACCOUNT;

			
		}
		else
			return SLURM_SUCCESS;


	}
	else
		return SLURM_SUCCESS;

	
}

extern int job_modify(struct job_descriptor *job_desc,
		      struct job_record *job_ptr, uint32_t submit_uid)
{
	return SLURM_SUCCESS;
}
