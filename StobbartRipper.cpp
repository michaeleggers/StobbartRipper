
#include <stdio.h>
#include <stdint.h>
#include <vector>

#include "platform.h"

#define MAX_LABEL_SIZE (31+1)

struct MEFile
{
	void* data;
	uint32_t len;
	uint32_t cursor;
};

MEFile MEFileInit(void* data, uint32_t len)
{
	MEFile meFile = {};
	meFile.cursor = 0;
	meFile.len = len;
	meFile.data = malloc(len);
	memcpy(meFile.data, data, len);
	
	return meFile;
}

uint32_t meFile_readU32(MEFile* file)
{
	void* currentData = ((uint8_t*)file->data) + file->cursor;
	uint32_t value = *(uint32_t*)currentData;
	file->cursor += 4;

	return value;
}

void meFile_read(MEFile* file, void* in_out_buffer, uint32_t len)
{
	void* currentData = ((uint8_t*)file->data) + file->cursor;
	memcpy(in_out_buffer, currentData, len);
	file->cursor += len;
}

struct MemHandle
{
	void*    data;
	uint32_t size;
	uint32_t refCount;
	uint16_t cond;
	MemHandle* next, * prev;
};

struct Res
{
	void* data;
	uint32_t size;
};

struct Grp 
{
	uint32_t   noRes;
	MemHandle* resHandle;
	uint32_t*  offset;
	uint32_t*  length;
};

struct Clu 
{
	uint32_t refCount;
	void* dummyPtr;
	char label[MAX_LABEL_SIZE];
	uint32_t noGrp;
	Grp* grp;
	Clu* nextOpen;
};

struct Sprite
{
	char resourceType[6];
	uint16_t version;		// ?
	uint32_t length;		// Including this header!
	char someInfo[4];		// Maybe compression type?
};

int main(int argc, char** argv)
{
	uint8_t* swordresData;
	int swordresSize;
	read_file("bsdata/SWORDRES.RIF", &swordresData, &swordresSize);

	MEFile swordresFile = MEFileInit(swordresData, swordresSize);
	uint32_t numClus = meFile_readU32(&swordresFile);

	Clu* clus = new Clu[numClus]();

	uint32_t* cluIndex = (uint32_t*)malloc(numClus * 4);
	meFile_read(&swordresFile, cluIndex, numClus * 4);

	printf("Clusters:\n");
	for (uint32_t clusCount = 0; clusCount < numClus; clusCount++) {
		printf("Cluster Number: %d\n", clusCount);
		if (cluIndex[clusCount]) {
			Clu* cluster = clus + clusCount;
			meFile_read(&swordresFile, cluster->label, MAX_LABEL_SIZE);
			printf("  label: %s\n", cluster->label);

			cluster->dummyPtr = NULL;
			cluster->noGrp = meFile_readU32(&swordresFile);
			printf("  # groups: %d\n", cluster->noGrp);
			cluster->grp = new Grp[cluster->noGrp];
			cluster->nextOpen = NULL;
			memset(cluster->grp, 0, cluster->noGrp * sizeof(Grp));
			cluster->refCount = 0;

			uint32_t* grpIndex = (uint32_t*)malloc(cluster->noGrp * 4);
			meFile_read(&swordresFile, grpIndex, cluster->noGrp * 4);

			uint32_t totalResCount = 0;
			printf("  Groups:\n");
			for (uint32_t grpCount = 0; grpCount < cluster->noGrp; grpCount++) {
				printf("    Group %d:\n", grpCount);
				if (grpIndex[grpCount]) {
					Grp* group = cluster->grp + grpCount;
					group->noRes = meFile_readU32(&swordresFile);
					group->resHandle = new MemHandle[group->noRes];
					group->offset = new uint32_t[group->noRes];
					group->length = new uint32_t[group->noRes];
					totalResCount += group->noRes;
					uint32_t* resIdIdx = (uint32_t*)malloc(group->noRes * 4);
					meFile_read(&swordresFile, resIdIdx, group->noRes * 4);

					printf("      # resources: %d\n", group->noRes);
					for (uint32_t resCount = 0; resCount < group->noRes; resCount++) {
						if (resIdIdx[resCount]) {
							group->offset[resCount] = meFile_readU32(&swordresFile);
							group->length[resCount] = meFile_readU32(&swordresFile);
						}
						else {
							group->offset[resCount] = 0xFFFFFFFF;
							group->length[resCount] = 0;
						}
						printf("        offset: %d\n", group->offset[resCount]);
						printf("        length: %d\n", group->length[resCount]);
					}
				}
			}
			printf("      # resources total: %d\n", totalResCount);
		}
	}


	/* Read Cluster Files */
	std::vector<Res> resources;
	uint8_t* generalCLUData;
	int generalCLUSize;
	read_file("bsdata/GENERAL.CLU", &generalCLUData, &generalCLUSize);
	MEFile cluFile = MEFileInit(generalCLUData, generalCLUSize);

	Clu* generalCluster = &clus[3];
	for (uint32_t grpCount = 0; grpCount < generalCluster->noGrp; grpCount++) {
		Grp * grp = &generalCluster->grp[grpCount];
		for (uint32_t resCount = 0; resCount < grp->noRes; resCount++) {
			uint32_t resOffset = grp->offset[resCount];
			uint32_t resLength = grp->length[resCount];
			Res res = {};
			res.data = malloc(resLength);
			res.size = resLength;
			meFile_read(&cluFile, res.data, resLength);
			resources.push_back(res);
		}
	}

	/* Write Resources to disk */
	
	for (uint32_t resCount = 0; resCount < resources.size(); resCount++) {		
		char filename[64] = {};
		sprintf(filename, "output/resource_%d.bin", resCount);
		write_file(filename, resources[resCount].data, resources[resCount].size);
	}
	

	/* Cleanup */
	delete[] clus;

	return 0;
}
