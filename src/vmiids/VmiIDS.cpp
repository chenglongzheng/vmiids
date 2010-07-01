/*
 * VmiIDS.cpp
 *
 *  Created on: Jun 26, 2010
 *      Author: kittel
 */

#include "VmiIDS.h"

#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>

#include "vmiids_rpc.h"

#include "SimpleDetectionModule.h"

#define DEBUG

#ifdef DEBUG
#define VERBOSE "VmiIDS"
#endif /* DEBUG */
#include "../libvmi/Debug.h"

void terminate_handler(int signum) {
	VmiIDS::getInstance()->stopIDS(signum);
}

void terminate_rpcListener(int signum) {
	pthread_exit(NULL);
}

extern "C" int rpc_main(void);

void * rpcThreadFunction(void * argument) {
	struct sigaction terminate_action;

	/* Set up the structure to specify the new action. */
	terminate_action.sa_handler = terminate_rpcListener;
	sigemptyset(&terminate_action.sa_mask);
	terminate_action.sa_flags = 0;

	sigaction(SIGINT, &terminate_action, NULL);
	sigaction(SIGHUP, &terminate_action, NULL);
	sigaction(SIGTERM, &terminate_action, NULL);

	rpc_main();
	pthread_exit(NULL);
}

int main(int argc, char ** argv) {

	struct sigaction terminate_action;

	/* Set up the structure to specify the new action. */
	terminate_action.sa_handler = terminate_handler;
	sigemptyset(&terminate_action.sa_mask);
	terminate_action.sa_flags = 0;

	sigaction(SIGINT, &terminate_action, NULL);
	sigaction(SIGHUP, &terminate_action, NULL);
	sigaction(SIGTERM, &terminate_action, NULL);

	VmiIDS::getInstance()->startIDS();

	VmiIDS::getInstance()->waitIDS();

	exit(0);
}

#include "vmiids_rpc.h"

bool_t enqueuedetectionmodule_1_svc(char *arg1, int *result,
		struct svc_req *rqstp) {
	VmiIDS::getInstance()->enqueueDetectionModule(arg1);
	return 1;
}

bool_t dequeuedetectionmodule_1_svc(char *arg1, int *result,
		struct svc_req *rqstp) {
	VmiIDS::getInstance()->dequeueDetectionModule(arg1);
	return 1;
}

bool_t enqueuenotificationmodule_1_svc(char *arg1, int *result,
		struct svc_req *rqstp) {
	VmiIDS::getInstance()->enqueueNotificationModule(arg1);
	return 1;
}

bool_t dequeuenotificationmodule_1_svc(char *arg1, int *result,
		struct svc_req *rqstp) {
	VmiIDS::getInstance()->dequeueNotificationModule(arg1);
	return 1;
}

void * stopIDSThreadFunction(void * nothing) {
	sleep(1);
	VmiIDS::getInstance()->stopIDS(SIGTERM);
}

bool_t loadsharedobject_1_svc(char *arg1, int *result, struct svc_req *rqstp) {
	VmiIDS::getInstance()->loadSharedObject(arg1);
	return 1;
}

bool_t stopids_1_svc(int *result, struct svc_req *rqstp) {
	pthread_t killThread;
	pthread_create(&killThread, NULL, &stopIDSThreadFunction, NULL);
	return 1;
}

int simp_prog_1_freeresult(SVCXPRT *transp, xdrproc_t xdr_result,
		caddr_t result) {
	xdr_free(xdr_result, result);

	/*
	 * Insert additional freeing code here, if needed
	 */

	return 1;
}

VmiIDS* VmiIDS::instance = NULL;

VmiIDS::VmiIDS() :
	detectionModules(), notificationModules(), sensorModules() {
	this->vmiRunning = false;
	pthread_mutex_init(&detectionModuleMutex, NULL);
	pthread_mutex_init(&notificationModuleMutex, NULL);
	pthread_mutex_init(&sensorModuleMutex, NULL);
}

VmiIDS::~VmiIDS() {
	pthread_mutex_lock(&detectionModuleMutex);
	pthread_mutex_lock(&notificationModuleMutex);
	pthread_mutex_lock(&sensorModuleMutex);

	while (!detectionModules.empty()) {
		delete (detectionModules.begin()->second);
		detectionModules.erase(detectionModules.begin());
	}
	while (!notificationModules.empty()) {
		delete (notificationModules.begin()->second);
		notificationModules.erase(notificationModules.begin());
	}
	while (!sensorModules.empty()) {
		delete (sensorModules.begin()->second);
		sensorModules.erase(sensorModules.begin());
	}
}

VmiIDS *VmiIDS::getInstance() {
	if (!instance)
		instance = new VmiIDS();
	return instance;
}

int VmiIDS::startIDS() {
	if (this->vmiRunning == true) {
		printf("IDS already running");
		return 0;
	}
	printf("IDS Starting\n");
	this->vmiRunning = true;
	int rpcRet = pthread_create(&rpcThread, NULL, rpcThreadFunction, NULL);
	int vmiidsRet = pthread_create(&vmiidsThread, NULL, VmiIDS::run,
			(void*) this);
}

void * VmiIDS::run(void * this_pointer) {
	VmiIDS * this_p = (VmiIDS *) this_pointer;

	this_p->enqueueDetectionModule("SimpleDetectionModule");
	this_p->loadSharedObjectsInitial("/home/kittel/workspace/libvmi/src/vmiids/.libs");
	//this_p->loadSharedObjectsInitial("/home/kittel/workspace/libvmi/src/vmiidsmodules/sensor/.libs");
	//this_p->loadSharedObjectsInitial("/home/kittel/workspace/libvmi/src/vmiidsmodules/notification/.libs");
	//this_p->loadSharedObjectsInitial("/home/kittel/workspace/libvmi/src/vmiidsmodules/detection/.libs");

	while (this_p->vmiRunning) {
		pthread_mutex_lock(&this_p->detectionModuleMutex);

		for (std::map<std::string, DetectionModule*>::iterator it =
				this_p->detectionModules.begin(); it
				!= this_p->detectionModules.end(); ++it) {
			it->second->run();
		}
		pthread_mutex_unlock(&this_p->detectionModuleMutex);
		sched_yield();
	}
}

void VmiIDS::loadSharedObjectsInitial(std::string path) {

	DIR *dp;
	struct dirent *dirp;
	if ((dp = opendir(path.c_str())) == NULL) {
		std::cout << "Error opening " << path << std::endl;
		return;
	}

	while ((dirp = readdir(dp)) != NULL) {
		if (strstr(dirp->d_name, ".so") != NULL) {
			std::string filename = path;
			filename.append("/");
			filename.append(dirp->d_name);
			struct stat fileStat;
			lstat(filename.c_str(), &fileStat);
			if (!S_ISLNK(fileStat.st_mode)) {
				void *dlib;
				dlib = dlopen(filename.c_str(), RTLD_NOW | RTLD_GLOBAL);
				if (dlib == NULL) {
					std::cerr << dlerror() << std::endl;
					continue;
				}
			}
		}
	}
	closedir(dp);
	return;

}

void VmiIDS::loadSharedObject(std::string path) {
	printf("Trying to load shared object at: %s\n", path.c_str());

	void *dlib;
	dlib = dlopen(path.c_str(), RTLD_NOW | RTLD_GLOBAL);
	if (dlib == NULL) {
		std::cerr << dlerror() << std::endl;
		exit(-1);
	}
}

void VmiIDS::waitIDS() {
	pthread_join(vmiidsThread, NULL);
}

void VmiIDS::stopIDS(int signum) {
	printf("IDS Stopping\n");
	this->vmiRunning = false;
	pthread_kill(this->rpcThread, SIGTERM);
	pthread_join(this->rpcThread, NULL);
	pthread_join(this->vmiidsThread, NULL);
}

void VmiIDS::enqueueDetectionModule(DetectionModule *detectionModule) {
	pthread_mutex_lock(&detectionModuleMutex);
	detectionModules[detectionModule->getName()] = detectionModule;
	pthread_mutex_unlock(&detectionModuleMutex);
}

bool VmiIDS::enqueueDetectionModule(std::string detectionModuleName) {
	if (detectionModuleName.find("SimpleDetectionModule") != std::string::npos) {
		pthread_mutex_lock(&detectionModuleMutex);
		detectionModules[detectionModuleName] = new SimpleDetectionModule();
		pthread_mutex_unlock(&detectionModuleMutex);
	}
}

bool VmiIDS::dequeueDetectionModule(std::string detectionModuleName) {
	pthread_mutex_lock(&detectionModuleMutex);
	for (std::map<std::string, DetectionModule*>::iterator it =
			this->detectionModules.begin(); it != this->detectionModules.end(); ++it) {
		if (it->first.compare(detectionModuleName) == 0) {
			this->detectionModules.erase(it);
		}
	}
	pthread_mutex_unlock(&detectionModuleMutex);
}

void VmiIDS::enqueueNotificationModule(NotificationModule *notificationModule) {
	pthread_mutex_lock(&notificationModuleMutex);
	notificationModules[notificationModule->getName()] = notificationModule;
	pthread_mutex_unlock(&notificationModuleMutex);
}

bool VmiIDS::enqueueNotificationModule(std::string notificationModuleName) {
	if (notificationModuleName.find("") != std::string::npos) {
		pthread_mutex_lock(&notificationModuleMutex);
		//notificationModules[notificationModuleName] = new SimpleDetectionModule();
		pthread_mutex_unlock(&notificationModuleMutex);
	}
}

bool VmiIDS::dequeueNotificationModule(std::string notificationModuleName) {
	pthread_mutex_lock(&notificationModuleMutex);
	for (std::map<std::string, NotificationModule*>::iterator it =
			this->notificationModules.begin(); it
			!= this->notificationModules.end(); ++it) {
		if (it->first.compare(notificationModuleName) == 0) {
			this->notificationModules.erase(it);
		}
	}
	pthread_mutex_unlock(&notificationModuleMutex);
}

void VmiIDS::enqueueSensorModule(SensorModule *sensorModule) {
	pthread_mutex_lock(&sensorModuleMutex);
	sensorModules[sensorModule->getName()] = sensorModule;
	pthread_mutex_unlock(&sensorModuleMutex);
}

NotificationModule *VmiIDS::getNotificationModule(
		std::string notificationModuleName) {
	return this->notificationModules[notificationModuleName];
}

SensorModule *VmiIDS::getSensorModule(std::string sensorModuleName) {
	return this->sensorModules[sensorModuleName];
}

void VmiIDS::collectThreadLevel() {
}
