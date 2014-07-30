/*****************************************************************************\
 *  job_submit_plg_pozo.c - Implements POZO policy for PL-Grid.
 *****************************************************************************
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

#include "slurm/slurm_errno.h"
#include "src/common/slurm_xlator.h"
#include "src/slurmctld/slurmctld.h"

//Library needed to communicate with ldap server
#include<ldap.h>

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
const char plugin_name[]       	= "PL-Grid POZO Implementation Plugin";
const char plugin_type[]       	= "job_submit/plg_pozo";
const uint32_t plugin_version   = 100;
const uint32_t min_plug_version = 100;

/*****************************************************************************\
 * We've provided a simple example of the type of things you can do with this
 * plugin. If you develop another plugin that may be of interest to others
 * please post it to slurm-dev@lists.llnl.gov  Thanks!
\*****************************************************************************/


char* getDefaultGrant (char *);
char ** getUserGrants(char *uid);



int init (void)
{
	info("[ PLG ]:: initialization of '%s'", plugin_name);
	info("[ PLG ]:: plugin '%s' is ready", plugin_name);
	return SLURM_SUCCESS;
}


void fini (void)
{
	info("[ PLG ]:: finishing '%s'", plugin_name);
}


extern int job_submit(struct job_descriptor *job_desc, uint32_t submit_uid ,char **err_msg)
{
	struct passwd *pw;	
	int i;
	/* Log select fields from a job submit request. See slurm/slurm.h
	 * for information about additional fields in struct job_descriptor.
	 * Note that default values for most numbers is NO_VAL */
	info("[ PLG ]:: Job submit request: account:'%s' begin_time:'%ld' dependency:'%s' "
	     "name:'%s' partition:'%s' qos:'%s' submit_uid:'%u' time_limit:'%u' "
	     "user_id:'%u'",
	     job_desc->account, (long)job_desc->begin_time,
	     job_desc->dependency,
	     job_desc->name, job_desc->partition, job_desc->qos,
	     submit_uid, job_desc->time_limit, job_desc->user_id);

	int plgPrefixPartitionCmp = strncmp(job_desc->partition, "plgrid", 6);
	if (plgPrefixPartitionCmp == 0)
	{
		info("[ PLG ]:: set account '%s' for uid '%u'", job_desc->account, job_desc->user_id);
		if (submit_uid!=0)
		{
		pw = getpwuid (submit_uid);
			if (!pw) 
			{
				info( "[PLG-POZO]: Error looking up uid=%u in passwd",submit_uid);
				return SLURM_ERROR;
			}
		
			if(job_desc->account == NULL)  
			{
				char * defGrant;
	
				defGrant=getDefaultGrant(pw->pw_name);				
				job_desc->account=xstrdup(defGrant);
				debug("[PLG-POZO] Defult grant set to:%s",job_desc->account);
			}

			char** userGrants;
			userGrants=getUserGrants(pw->pw_name);
			debug2("[PLG-POZO]userGrants finished successfully");
			for(i=0;userGrants[i]!=NULL;i++)
			{
				debug("[PLG-POZO] User grant is:%s comparing with %s",job_desc->account,userGrants[i]);
				if (strcmp(userGrants[i],job_desc->account)==0)
				{
					debug("[PLG-POZO] User %s allowd",job_desc->account);
					return SLURM_SUCCESS;
				}
			}
		}
		else
			return SLURM_SUCCESS;

	}
	else
		return SLURM_SUCCESS;
	info("Refused account:%s for user:%s",job_desc->account,pw->pw_name);
	return ESLURM_INVALID_ACCOUNT;
}

extern int job_modify(struct job_descriptor *job_desc,
		      struct job_record *job_ptr, uint32_t submit_uid)
{
	/* Log select fields from a job modify request. See slurm/slurm.h
	 * for information about additional fields in struct job_descriptor.
	 * Note that default values for most numbers is NO_VAL */
	info("[ PLG ]:: Job modify request: account:'%s' begin_time:'%ld' dependency:'%s' "
	     "job_id:'%u' name:'%s' partition:'%s' qos:'%s' submit_uid:'%u' "
	     "time_limit:'%u'",
	     job_desc->account, (long)job_desc->begin_time,
	     job_desc->dependency,
	     job_desc->job_id, job_desc->name, job_desc->partition,
	     job_desc->qos, submit_uid, job_desc->time_limit);

	return SLURM_SUCCESS;
}


/*
Function returns pointer to string with default grant id, for user with given uid (for example plgstolarek) 
space for returned string  will be dynamically alocated, you have to return memory with ldap_value_free(pointer)
*/
char * getDefaultGrant(char *uid)
{
 LDAP *ld;
 int desired_version = LDAP_VERSION3;
 char *ldap_host     = "ldap://192.168.69.110";
 char *FIND_DN  = "dc=icm,dc=plgrid,dc=pl";

 LDAPMessage  *result, *e;
 BerElement   *ber;
 char         *a;
 char         **vals;
 int          i, rc;

 ldap_initialize( &ld, ldap_host );
 printf("LDAP initialize successful.\n");

 rc = ldap_set_option(ld, LDAP_OPT_PROTOCOL_VERSION, &desired_version);
 if ( rc != LDAP_SUCCESS ) 
 {
    perror( "ldap_set_option failed" );
    return NULL;
 } 
 else 
 {
    printf("Set LDAPv3 client version.\n");
 }

 rc = ldap_bind_s(ld,NULL ,NULL,LDAP_AUTH_SIMPLE);
 if ( rc != LDAP_SUCCESS ) {
    fprintf(stderr, "ldap_simple_bind_s: %s\n", ldap_err2string(rc));
    return NULL;
 } 
 else 
 {
    printf("LDAP connection successful.\n");
 }

 char * attrs[]={"plgridDefaultGrantID",NULL};
 //TODO NA WIEKI - filtr[100]
 char  filtr[100];
 sprintf(filtr,"(uid=%s)",uid);
 fprintf(stderr,"filtr:%s\n",filtr);

 if ( ( rc = ldap_search_ext_s( ld, FIND_DN, LDAP_SCOPE_SUBTREE,
                                filtr,attrs,0,NULL,NULL,NULL,
                                LDAP_NO_LIMIT, &result ) ) != LDAP_SUCCESS ) 
 {

     fprintf(stderr, "ldap_search_ext_s: %s\n", ldap_err2string(rc));
     ldap_unbind( ld );
     return NULL;
 }

 if( ldap_count_entries(ld,result) > 1 )
        {
                error("[PLG-POZO]Too many results, expected only one. Looks like uid is not unique\n");
                return NULL;
        }
 else if (ldap_count_entries(ld,result) == 0)
        {
                error("[PLG-POZO]Uid not found\n");
                ldap_unbind (ld);
                return NULL;
        }

 fprintf(stderr,"Checking default grant in ldap\n");
 char ** defaultGrant=NULL;
 if( (defaultGrant = ldap_get_values(ld,result,attrs[0])) != NULL)
         fprintf (stderr,"Default grant id is %s",defaultGrant[0]);
 else   
        info("ldap_get_values:");

 ldap_msgfree( result );
 ldap_unbind( ld );
 return defaultGrant[0];
}


/*
Function returns private grant of user (given by uid=login)
space for returned string  will be dynamically alocated, you have to return memory with ldap_value_free(pointer)
*/

char * getPrivateGrant(char *uid)
{
 LDAP *ld;
 int desired_version = LDAP_VERSION3;
 char *ldap_host     = "ldap://192.168.69.110";
 char *FIND_DN  = "dc=icm,dc=plgrid,dc=pl";

 LDAPMessage  *result, *e;
 BerElement   *ber;
 char         *a;
 char         **vals;
 int          i, rc;

 ldap_initialize( &ld, ldap_host );
 debug("[PLG-POZO]LDAP initialize successful.\n");

 rc = ldap_set_option(ld, LDAP_OPT_PROTOCOL_VERSION, &desired_version);
 if ( rc != LDAP_SUCCESS ) {
    error( "[PLG-POZO]ldap_set_option failed" );
    return NULL;
 } else {
    debug("[PLG-POZO]Set LDAPv3 client version.\n");
 }

 rc = ldap_bind_s(ld,NULL ,NULL,LDAP_AUTH_SIMPLE);
 if ( rc != LDAP_SUCCESS ) {
    error("[PLG-POZO]ldap_simple_bind_s: %s\n", ldap_err2string(rc));
    return NULL;
 } else {
    debug("[PLG-POZO]LDAP connection successful.\n");
 }

 char * attrs[]={"plgridPrivateGrantID",NULL};
 //TODO NA WIEKI - filtr[100]
 char  filtr[100];
 sprintf(filtr,"(uid=%s)",uid);
 debug("[PLG-POZO]filtr:%s\n",filtr);

 if ( ( rc = ldap_search_ext_s( ld, FIND_DN, LDAP_SCOPE_SUBTREE,
                                filtr,attrs,0,NULL,NULL,NULL,
                                LDAP_NO_LIMIT, &result ) ) != LDAP_SUCCESS ) {

     debug("[PLG-POZO]ldap_search_ext_s: %s\n", ldap_err2string(rc));
     ldap_unbind( ld );
     return NULL;
 }

 BerValue ** privateGrant;
 privateGrant = ldap_get_values_len(ld,result,attrs[0]);
 int numberOfValues =ldap_count_values_len(privateGrant);
 debug2("[PLG-POZO]Number of private grants:%d\n",numberOfValues);
 if(numberOfValues==0)
 {
        info("User doesn't have active private grant\n");
 ldap_msgfree( result );
 ldap_unbind( ld );

        return NULL;
 }
 else
 {
        char ** privGrant=ldap_get_values(ld,result,attrs[0]);
	 ldap_msgfree( result );
	 ldap_unbind( ld );
        debug("[PLG-POZO]Private grant:%s\n",privGrant[0]);
        return privGrant[0];
 }
}

char ** getUserGrants(char *uid)
{
 LDAP *ld;
 int desired_version = LDAP_VERSION3;
 char *ldap_host     = "ldap://192.168.69.110";
 char *FIND_DN  = "dc=icm,dc=plgrid,dc=pl";

 LDAPMessage  *result, *e;
 BerElement   *ber;
 char         *a;
 char         **vals;
 int          i, rc;

 ldap_initialize( &ld, ldap_host );
 debug("[PLG-POZO]LDAP initialize successful.\n");

 rc = ldap_set_option(ld, LDAP_OPT_PROTOCOL_VERSION, &desired_version);
 if ( rc != LDAP_SUCCESS ) {
    error( "[PLG-POZO]ldap_set_option failed" );
    return NULL;
 } else {
    debug("[PLG-POZO]Set LDAPv3 client version.\n");
 }

 rc = ldap_bind_s(ld,NULL ,NULL,LDAP_AUTH_SIMPLE);
 if ( rc != LDAP_SUCCESS ) {
    error("ldap_simple_bind_s: %s\n", ldap_err2string(rc));
    return NULL;
 } else {
    debug("[PLG-POZO]LDAP connection successf/ul.\n");
 }

 char * attrs[]={"plgridGrantID",NULL};
 //TODO NA WIEKI - filtr[100]
 char  filtr[100];
 sprintf(filtr,"(memberUid=%s)",uid);
 debug("[PLG-POZO] filtr:%s\n",filtr);


 int numberOfEntries=1;
 if ( ( rc = ldap_search_ext_s( ld, FIND_DN, LDAP_SCOPE_SUBTREE,
                                filtr,attrs,0,NULL,NULL,NULL,
                                LDAP_NO_LIMIT, &result ) ) != LDAP_SUCCESS ) {

     debug("[PLG-POZO] ldap_search_ext_s: %s\n", ldap_err2string(rc));
     ldap_unbind( ld );
     numberOfEntries=0;
 }

 if(numberOfEntries!=0)
	 numberOfEntries=ldap_count_entries(ld,result);

 debug("[PLG-POZO]Number of entries:%d\n",numberOfEntries);
 //Alokujemy tablice wskaznikow do stringow, zaczynamy od jednego - na grant osobisty 
 char **userGrants = (char **)malloc( sizeof(char *));
 char * prvGrant=getPrivateGrant(uid);
 userGrants[0]=xstrdup(prvGrant);

 int currentGrant=1;

 LDAPMessage * iterator=NULL;
 for(iterator=result;iterator!=NULL;iterator=ldap_next_entry(ld,iterator))
 {



	 BerValue ** realGrant;;
         realGrant = ldap_get_values_len(ld,iterator,attrs[0]);
         int numberOfValues =ldap_count_values_len(realGrant);
         debug("[PLG-POZO] Number of real grants:%d\n",numberOfValues);
         int  iterator=0;
         for(iterator=0;iterator<numberOfValues;iterator++)
         {
                debug("[PLG-POZO]Real grant is:%s\n",realGrant[iterator]->bv_val);
                //alokujemy miejsce na granty wlasciwe
                userGrants=realloc(userGrants,sizeof(char*)*(currentGrant+2));
                if(!userGrants)
                {
                        debug("[PLG-POZO]realloc error\n");
                }
                userGrants[currentGrant]=malloc(sizeof(char)*(realGrant[iterator]->bv_len+1));
                if(!userGrants[currentGrant])
                {
                        debug("[PLG-POZO]malloc error\n");
                }
                        strcpy(userGrants[currentGrant], realGrant[iterator]->bv_val);
                currentGrant++;
         }



 }

 userGrants[currentGrant]=NULL;
 for (i=0;i<currentGrant;i++)
 {
        debug("[PLG-POZO] User grant number:%d is: %s\n", i,userGrants[i]);
 }


 return userGrants;

}


