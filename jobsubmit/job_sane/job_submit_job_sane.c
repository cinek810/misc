/*****************************************************************************\
 *  job_submit_job_sane.c - Check if job submit request is sain.
 *****************************************************************************
Author: Marcin Stolarek (Interdycyplinary Center for Mathematical 
	and Computational Modeling. University of Warsaw)
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




#include <pwd.h>

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
const char plugin_name[]       	= "Job submit job specification saintizationt plugin";
const char plugin_type[]       	= "job_submit/job_sane";
const uint32_t plugin_version   = 110;
const uint32_t min_plug_version = 100;

/*****************************************************************************\
 * We've provided a simple example of the type of things you can do with this
 * plugin. If you develop another plugin that may be of interest to others
 * please post it to slurm-dev@schedmd.com  Thanks!
\*****************************************************************************/

//Pointer to stored information about tags, cores and user list?
char ** tags;
int * cores,*users;

#define LINE_SIZE 200

int init()
{	
	int i,number_of_lines=0;
	char  *config,*tmp=NULL;
	char line[LINE_SIZE],feature[50],keyword[50];
	FILE * fr;
	int number_of_features=0,number_of_users=0;

	config=get_extra_conf_path("job_sane.conf");
	
	info("job_sane:Trying to read configuration file %s",config);
	
	fr = fopen (config, "r");
	if(fr==NULL)
		fatal("job_sane:Cannot open configuration file");	

	info("job_sane:counting lines");
	while (fgets(line,LINE_SIZE,fr)!=NULL)
		number_of_lines++;
	info("job_sane:count:%d",number_of_lines);

	fseek(fr, 0, SEEK_SET);
	
	//TODO
	//This is very ugly solution and obviously it's waisting memory
	//number_of_lines=number_of_users+number_of_features
	//
	//
	tags=(char**)xmalloc(sizeof(char *) * (number_of_lines+1));
	cores=xmalloc(sizeof(int)*number_of_lines);
	users=xmalloc(sizeof(int) * (number_of_lines+1));

	
	for (i=0;i<number_of_lines;i++)
	{
		fgets(line,LINE_SIZE,fr);
		sscanf(line,"%s",&keyword);
		if(!strcmp(keyword,"feature"))
		{
			sscanf(line,"%*s %s %d",&feature,&cores[i]);
			debug("job_sane:feature=%s cores=%d",feature,cores[i]);
			tags[number_of_features]=xmalloc(sizeof(char)*(strlen(feature)+1));
			strcpy(tags[number_of_features],feature);
			number_of_features++;
		}
		else if(!strcmp(keyword,"user"))
		{
			struct passwd* userData;
			sscanf(line,"%*s %s",feature);
			int uid=getpwnam(feature)->pw_uid;
			if ( uid==0 )
			{
				debug("job_sane: user_name=root cannot be added to the accepted users - root is always accepted.");
				continue;
			}
			users[number_of_users]=uid;
			debug("job_sane:user_name=%s adding to accepted users with uid=%d",feature,users[number_of_users]);
			number_of_users++;
		}
		else
		{
			error("job_sane:Unknown option in configuration file :%s",keyword);
		}

	}
	tags[number_of_features]=NULL;
	users[number_of_users]=0;
	fclose(fr);

	info("job_sane initialization finished");

	return SLURM_SUCCESS;

}


int fini()
{
	int i=0;
	while(tags[i])
	{
		xfree(tags[i]);
		i++;
	}
	xfree(users);
	xfree(cores);
	return SLURM_SUCCESS;
}



int _add_feature(char ** where_to_add,char* what_to_add)
{
	debug("job_sane: _add_feature: adding feature to %s ",*where_to_add);
	if(*where_to_add==NULL)
	{	
		debug("job_sane: _add_feature: adding first feature ");
		*where_to_add=xmalloc(1000);
		strcat(*where_to_add,what_to_add);
	}
	else
	{
		debug2("job_sane: _add_feature: adding consecutive feature");
		strcat(*where_to_add,"|");
		strcat(*where_to_add,what_to_add);
	}
	debug("job_sane: _add_feature: feature list is %s ",*where_to_add);

	return 0;
}

extern int job_submit(struct job_descriptor *job_desc, uint32_t submit_uid,
		      char **err_msg)
{
	int i=0;


	info("job_sane: job_submit: account:%s begin_time:%ld dependency:%s "
	     "name:%s partition:%s qos:%s submit_uid:%u time_limit:%u "
	     "user_id:%u, mem:%d",
	     job_desc->account, (long)job_desc->begin_time,
	     job_desc->dependency,
	     job_desc->name, job_desc->partition, job_desc->qos,
	     submit_uid, job_desc->time_limit, job_desc->user_id,
	     job_desc->pn_min_memory	);
	

	/*Doesnt check jobs from users mensioned in configuration file*/
	while(users[i]!=0)
	{
		if(submit_uid==users[i])
		{
			verbose("Allowing job from uid:%d without job checking",users[i]);
			return SLURM_SUCCESS;
		}
		i++;
	}

	if(submit_uid==0)
		return SLURM_SUCCESS;

	if(job_desc->pn_min_memory==0)
	{
		*err_msg=xstrdup("You are not allowed to submit job without memory specification. Specify --mem different than 0");
		return ESLURM_REQUESTED_NODE_CONFIG_UNAVAILABLE;
	}


	/*Set up some defaults assumed by following logics*/
	if(job_desc->min_nodes==0 || job_desc->min_nodes==4294967294)
	{
		job_desc->min_nodes=1;
	}
	if(job_desc->max_nodes==0 || job_desc->min_nodes==4294967294)
	{
		job_desc->max_nodes=1;
	}

	if(job_desc->min_cpus==4294967294)
	{
		job_desc->min_cpus=(job_desc->min_nodes)*(job_desc->ntasks_per_node);
	}
	debug("job_sane: job_submit: Specified feature list:%s ",job_desc->features);
	
	int cpus_per_node=job_desc->min_cpus/job_desc->min_nodes;
	debug("job_sane: job_submit: Specified requirements: min_nodes=%u, min_cpus=%u, hence cpus_per_node=%d",job_desc->min_nodes,job_desc->min_cpus,cpus_per_node);
	
	if(job_desc->min_nodes>1)//Apply testing to multi node jobs only
	{
		if(job_desc->features==NULL) //No features specified	
		{
			debug2("job_sane: job_submit: No features specified");
			for(i=0;tags[i];i++)//Iterate through allowed features
			{
				debug2("job_sane: checking tag %s with %d cores per node",tags[i],cores[i]);
				if(cpus_per_node==cores[i]){
					debug("adding_feature:%s",tags[i]);
					_add_feature(&job_desc->features,tags[i]);
				}
		
			}
			if(job_desc->features==NULL)	{
				char * infoStr=xmalloc(1000);
				sprintf(infoStr,"Your job specification is very likely suboptimal.\nYou specified: Nodes=%u, Cores=%d, which gives %d  / %d = %d cpus per node. There are no %d cpu nodes in this cluster, try to allocate full nodes.\0",job_desc->min_nodes,job_desc->min_cpus, job_desc->min_cpus,job_desc->min_nodes,job_desc->min_cpus/job_desc->min_nodes,cpus_per_node);
				info("job_sane: Refusing job with message:%s",infoStr);
				*err_msg=infoStr;
				return ESLURM_REQUESTED_NODE_CONFIG_UNAVAILABLE;
			}
		}
		else{ //We have to check if specified features meet nodes and ppn requirements
		
			for(i=0;tags[i];i++){
				char * spec_feature;
				debug2("job_sane: Looking for tag:%s",tags[i]);
				spec_feature=strtok(job_desc->features,"|&");
				do{
					debug2("job_sane: Analysing feature spec_feature:%s",spec_feature);	
					if(strcmp(spec_feature,tags[i])==0){
						char * infoStr=xmalloc(1000);
						if(cpus_per_node!=cores[i]){
						sprintf(infoStr,"Your job specification is very likely suboptimal.\nYou specified: Nodes=%d, Cores=%d and feature=%s, which gives %d  / %d = %d cpus per node,while %s nodes have %d cpus per node.\0",job_desc->min_nodes,job_desc->min_cpus,spec_feature, job_desc->min_cpus,job_desc->min_nodes,cpus_per_node,spec_feature,cores[i]);
						info("job_sane: Refusing job with message:%s",infoStr);
						*err_msg=infoStr;
						return ESLURM_REQUESTED_NODE_CONFIG_UNAVAILABLE;
							
						}
					}
				}while(spec_feature=strtok(NULL,"|&"));
			}
		
		}
	}
		
	
	return SLURM_SUCCESS;
}

extern int job_modify(struct job_descriptor *job_desc,
		      struct job_record *job_ptr, uint32_t submit_uid)
{

	int i;



	/* Log select fields from a job modify request. See slurm/slurm.h
	 * for information about additional fields in struct job_descriptor.
	 * Note that default values for most numbers is NO_VAL */
	info("Job modify request: account:%s begin_time:%ld dependency:%s "
	     "job_id:%u name:%s partition:%s qos:%s submit_uid:%u "
	     "time_limit:%u",
	     job_desc->account, (long)job_desc->begin_time,
	     job_desc->dependency,
	     job_desc->job_id, job_desc->name, job_desc->partition,
	     job_desc->qos, submit_uid, job_desc->time_limit);

	debug("job_sane: job_modify: Specified feature list:%s ",job_desc->features);
	debug("job_sane: job_modify: Specified requirements: min_nodes=%d, min_cpus=%d",job_desc->min_nodes,job_desc->min_cpus);



        /*Doesnt check jobs from users mensioned in configuration file*/
        while(users[i]!=0)
        {
                if(submit_uid==users[i])
                {
                        verbose("Allowing job from uid:%d without job checking",users[i]);
                        return SLURM_SUCCESS;
                }
                i++;
        }

        if(submit_uid==0)
                return SLURM_SUCCESS;
	
	if(job_desc->min_nodes>1)//Apply testing to multi node jobs only
	{
		if(job_desc->features==NULL) //No features specified	
		{
			debug2("job_sane: job_modify: No features specified");
			for(i=0;tags[i];i++)//Iterate through allowed features
			{
				debug2("job_modify: checking tag %s",tags[i]);
				if(job_desc->min_cpus/job_desc->min_nodes==cores[i]	){
					debug("adding_feature:%s",tags[i]);
					_add_feature(&job_desc->features,tags[i]);
				}
		
			}
			if(job_desc->features==NULL)	{
				char * infoStr=xmalloc(1000);
				sprintf(infoStr,"Your job specification is very likely suboptimal.\nYou specified: Nodes=%d, Cores=%d, which gives %d  / %d = %d cpus per node. There are no %d cpu nodes in this cluster, try to allocate full nodes.\0",job_desc->min_nodes,job_desc->min_cpus, job_desc->min_cpus,job_desc->min_nodes,job_desc->min_cpus/job_desc->min_nodes,job_desc->min_cpus/job_desc->min_nodes);
				info("job_sane: job_modify:  Refusing job with message:%s",infoStr);
				return ESLURM_REQUESTED_NODE_CONFIG_UNAVAILABLE;
			}
		}
		else{ //We have to check if specified features meet nodes and ppn requirements
		
			for(i=0;tags[i];i++){
				char * spec_feature;
				debug2("job_sane: Looking for tag:%s",tags[i]);
				spec_feature=strtok(job_desc->features,"|&");
				do{
					debug2("job_sane: Analysing feature spec_feature:%s",spec_feature);	
					if(strcmp(spec_feature,tags[i])==0){
						int cpus_per_node=job_desc->min_cpus/job_desc->min_nodes;
						char * infoStr=xmalloc(1000);
						if(cpus_per_node!=cores[i]){
						sprintf(infoStr,"Your job specification is very likely suboptimal.\nYou specified: Nodes=%d, Cores=%d and feature=%s, which gives %d  / %d = %d cpus per node,while %s nodes have %d cpus per node.\0",job_desc->min_nodes,job_desc->min_cpus,spec_feature, job_desc->min_cpus,job_desc->min_nodes,cpus_per_node,spec_feature,cores[i]);
						info("job_sane: job_modify: Refusing job with message:%s",infoStr);
						return ESLURM_REQUESTED_NODE_CONFIG_UNAVAILABLE;
							
						}
					}
				}while(spec_feature=strtok(NULL,"|&"));
			}
		
		}
	}

	return SLURM_SUCCESS;
}
