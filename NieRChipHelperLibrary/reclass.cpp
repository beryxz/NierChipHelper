#include "pch.h"
#include "reclass.h"

void Chip::clear() {
	this->baseCode = -1;
	this->baseId = -1;
	this->type = -1;
	this->level = -1;
	this->weight = -1;
	this->offsetA = -1;
	this->offsetB = -1;
	this->offsetC = -1;
}
