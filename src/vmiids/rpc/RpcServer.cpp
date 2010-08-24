/*
 * RpcServer.cpp
 *
 *  Created on: Aug 24, 2010
 *      Author: kittel
 */

#include "RpcServer.h"

#include "vmiids/VmiIDS.h"

#include <rpc/pmap_clnt.h>
#include <signal.h>

vmi::RpcServer::RpcServer() {
	pthread_create(&this->rpcThread, NULL, rpcThreadFunction, NULL);
}

vmi::RpcServer::~RpcServer() {
	pthread_kill(this->rpcThread, SIGTERM);
	pthread_join(this->rpcThread, NULL);
}

void * vmi::RpcServer::rpcThreadFunction(void * argument) {
	argument = NULL;

	register SVCXPRT *transp;

	pmap_unset (VMIIDS_RPC, VMIIDS_RPC_VERSION);

	transp = svcudp_create(RPC_ANYSOCK);
	if (transp == NULL) {
		fprintf (stderr, "%s", "cannot create udp service.");
		exit(1);
	}
	if (!svc_register(transp, VMIIDS_RPC, VMIIDS_RPC_VERSION, vmi::RpcServer::dispatchRPC, IPPROTO_UDP)) {
		fprintf (stderr, "%s", "unable to register (VMIIDS_RPC_PROG, VMIIDS_RPC_VERSION, udp).");
		exit(1);
	}

	transp = svctcp_create(RPC_ANYSOCK, 0, 0);
	if (transp == NULL) {
		fprintf (stderr, "%s", "cannot create tcp service.");
		exit(1);
	}
	if (!svc_register(transp, VMIIDS_RPC, VMIIDS_RPC_VERSION, vmi::RpcServer::dispatchRPC, IPPROTO_TCP)) {
		fprintf (stderr, "%s", "unable to register (VMIIDS_RPC_PROG, VMIIDS_RPC_VERSION, tcp).");
		exit(1);
	}

	svc_run ();
	fprintf (stderr, "%s", "svc_run returned");
	/* NOTREACHED */

	pthread_exit(NULL);
}

void * vmi::RpcServer::stopIDSThreadFunction(void * nothing) {
	nothing = NULL;
	sleep(1);
	vmi::VmiIDS::getInstance()->stopIDS(SIGTERM);
	return NULL;
}

void vmi::RpcServer::dispatchRPC(struct svc_req *rqstp, register SVCXPRT *transp){
	union {
		const char *char_arg;
	} argument;
	char *result;
	static bool_t retbool;
	static char * returnStringMemory = 0;
	std::string returnString;
	xdrproc_t _xdr_argument, _xdr_result;

	memset ((char *)&argument, 0, sizeof (argument));

	switch (rqstp->rq_proc) {
	case NULLPROC:
		(void) svc_sendreply (transp, (xdrproc_t) xdr_void, (char *)NULL);
		return;

	case ENQUEUEDETECTIONMODULE:
		_xdr_argument = (xdrproc_t) xdr_wrapstring;
		_xdr_result = (xdrproc_t) xdr_bool;
		if (!svc_getargs (transp, (xdrproc_t) _xdr_argument, (caddr_t) &argument)) {
			svcerr_decode (transp);
			return;
		}
		retbool	= VmiIDS::getInstance()->enqueueDetectionModule(argument.char_arg);
		result = (char*) &retbool;
		break;

	case DEQUEUEDETECTIONMODULE:
		_xdr_argument = (xdrproc_t) xdr_wrapstring;
		_xdr_result = (xdrproc_t) xdr_bool;
		if (!svc_getargs (transp, (xdrproc_t) _xdr_argument, (caddr_t) &argument)) {
			svcerr_decode (transp);
			return;
		}
		retbool = VmiIDS::getInstance()->dequeueDetectionModule(argument.char_arg);
		result = (char*) &retbool;
		break;

	case RUNDETECTIONMODULE:
		_xdr_argument = (xdrproc_t) xdr_wrapstring;
		_xdr_result = (xdrproc_t) xdr_wrapstring;
		if (!svc_getargs (transp, (xdrproc_t) _xdr_argument, (caddr_t) &argument)) {
			svcerr_decode (transp);
			return;
		}
		returnString = VmiIDS::getInstance()->runSingleDetectionModule(argument.char_arg).c_str();
		if(returnStringMemory != NULL) free(returnStringMemory);
		returnStringMemory = (char *) malloc(returnString.length() + 1);
		memset(returnStringMemory, 0, returnString.length() + 1);
		memmove(returnStringMemory,returnString.c_str(), returnString.length());
		result = (char*) &returnStringMemory;
		break;

	case STOPIDS:
		_xdr_argument = (xdrproc_t) xdr_int;
		_xdr_result = (xdrproc_t) xdr_int;
		if (!svc_getargs (transp, (xdrproc_t) _xdr_argument, (caddr_t) &argument)) {
			svcerr_decode (transp);
			return;
		}
		pthread_t killThread;
		pthread_create(&killThread, NULL, &stopIDSThreadFunction, NULL);
		retbool = true;
		result = (char*) &retbool;
		break;

	case LOADSHAREDOBJECT:
		_xdr_argument = (xdrproc_t) xdr_wrapstring;
		_xdr_result = (xdrproc_t) xdr_bool;
		if (!svc_getargs (transp, (xdrproc_t) _xdr_argument, (caddr_t) &argument)) {
			svcerr_decode (transp);
			return;
		}
		retbool = VmiIDS::getInstance()->loadSharedObject(argument.char_arg);
		result = (char*) &retbool;
		break;

	default:
		svcerr_noproc (transp);
		return;
	}
	if (result != NULL && !svc_sendreply(transp, (xdrproc_t) _xdr_result, result)) {
		svcerr_systemerr (transp);
	}
	if (!svc_freeargs (transp, (xdrproc_t) _xdr_argument, (caddr_t) &argument)) {
		fprintf (stderr, "%s", "unable to free arguments");
		return;
	}

	//if (!simp_prog_1_freeresult (transp, _xdr_result, (caddr_t) &result))
	//	fprintf (stderr, "%s", "unable to free results");

	return;
}
