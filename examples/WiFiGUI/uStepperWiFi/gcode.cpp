#include "GCode.h"

GCode::GCode(void) {
	this->commandCount = 0;

	// Load default sender responses
	strcpy(this->validRes, GCODE_DEFAULT_VALID_RES);
	strcpy(this->errorRes, GCODE_DEFAULT_ERROR_RES);
}


void GCode::setSendFunc(void (*func)(char *data)){
	this->sendFunc = func;
}

void GCode::addCommand( char * command, void (*func)(char *command, char *data) ){
	// If no command is supplied, the function is used as default
	if( command == NULL ){
		this->defaultFunc = func;
		return;
	}

	// Prepare structure to hold command and function pointer
	command_func_t entry;

	entry.func = func;
	strcpy( entry.command, command );

	if(commandCount == 0){
		// Allocate memory to the first command in the list.
		this->commands = (command_func_t*)malloc( sizeof(command_func_t) );
	}else{
		// Reallocate memory, as more is needed
		this->commands = (command_func_t*)realloc(this->commands, (1 + this->commandCount) * sizeof(command_func_t));
	}

	// Add the command structure to the "array"
	this->commands[this->commandCount++] = entry;
}

void GCode::run( void ){
	// Process the latests bytes in buffer, and get the status of the incoming packet
	this->status = this->process();

	// If a new packet is ready
	if (this->status == GCODE_PACKET_READY) {

		// A packet is retrieved, loop through commands to see if any matches 
		for (uint8_t i = 0; i < commandCount; ++i)
		{
			char *command = this->commands[i].command;

			if( this->check( command ) ){
				// Return a response to sender
				// this->send(this->validRes, false);

				// We got a match, call the function pointer associated with this command
				this->commands[i].func( command, this->packet);
				return;
			}
		}

		// No match was found, call defaultFunc if defined
		if( this->defaultFunc != NULL )
			this->defaultFunc( NULL , this->packet );

	}else if ( this->status != GCODE_PACKET_NONE ){
		// Return a response to sender
		char buf[20] = {'\0'};

		strcat(buf, this->errorRes);
		strcat(buf, ": ");
		sprintf(buf + strlen(buf), "%d", this->status);

		this->send( buf, false );
	}
}

void GCode::printCommands( void ){
	char buf[MAX_PACKET_SIZE] = {'\0'};

	for (uint8_t i = 0; i < this->commandCount; ++i)
	{	
		strcat(buf, "- ");
		strcat(buf, this->commands[i].command);
		strcat(buf, "\n");
	}

	this->send(buf, false);
}

void GCode::enableCRC(bool state){
	this->useCRC = state;
}

bool GCode::check(char *cmd ) {
	return this->check( cmd, this->packet );
}

bool GCode::check(char *cmd, char *packet ) {
	char buf[10] = {'\0'};

	char *end = strpbrk(packet, " \n");

	size_t len;
	if (end == NULL) {
		len = strlen(packet);
	} else {
		len = end - packet;
	}

	if (len > sizeof(buf) - 1) {
		len = sizeof(buf) - 1;
	}

	memcpy(buf, packet, len);
	buf[len] = '\0';

	return strcmp(buf, cmd) == 0;
}

char *GCode::getPacket(void) {
	return this->packet;
}

uint8_t GCode::getStatus(void){
	return this->status;
}

bool GCode::value(char *name, float *var) {
	// If no packet is supplied, work on the latests in buffer
	return this->value(name, var, this->packet);
}

bool GCode::value(char *name, int32_t *var) {
	// If no packet is supplied, work on the latests in buffer
	return this->value(name, var, this->packet);
}

bool GCode::value(char *name, int32_t *var, char *packet ){
	float temp = 0.0;
  bool returnStatus;
	  returnStatus = this->value(name, &temp, packet );

	*var = (int32_t)temp;
	return returnStatus;
}

bool GCode::value(char *name, float *var, char *packet ) {
	char *start = strstr(packet, name);
	if (!start) {
		return false;
	}

	// Skip past the parameter name
	start++;

	char *end = strpbrk(start, " \n");

	size_t len;
	if (end == NULL) {
		len = strlen(start);
	} else {
		len = end - start;
	}

	if (len == 0) {
		return false;
	}

	char buf[24] = {'\0'};
	if (len > sizeof(buf) - 1) {
		len = sizeof(buf) - 1;
	}

	memcpy(buf, start, len);
	buf[len] = '\0';

	*var = atof(buf);
	return true;
}

void GCode::insert(char data){
	this->lastChar = micros();
	this->inPacket = true;

	// Save the char in the buffer ( but limited by MAX_PACKET_SIZE )
	if (this->packetLen < MAX_PACKET_SIZE - 1) {
		this->packet[this->packetLen++] = data;
	}
}

void GCode::insert(char *data){
	// Reset packet state and copy complete string
	this->packetLen = 0;
	memset(this->packet, 0, sizeof(this->packet));
	this->inPacket = true;

	strncpy(this->packet, data, MAX_PACKET_SIZE - 1);
	this->packetLen = strlen(this->packet);
	this->lastChar = micros();
}

uint8_t GCode::process(void){

	float crc;
	bool hasNewline = this->packetLen > 0 && this->packet[this->packetLen - 1] == '\n';

	// If no packet is incoming, and some old packet is still in the buffer clear the old packet
	if ( this->inPacket == false && this->packetLen > 0) {
		this->packetLen = 0;
		memset(this->packet, 0, sizeof(this->packet));
		return GCODE_PACKET_NONE;
	}

	// If the timeout has been reached, or a newline has been received the string is complete
	if ((micros() - lastChar >= PACKET_TIMEOUT || hasNewline) && inPacket == true) {
		inPacket = false;

		// Strip trailing newline so handlers receive clean strings
		if (hasNewline) {
			this->packet[this->packetLen - 1] = '\0';
		}

		if( !this->useCRC ){
			
			return GCODE_PACKET_READY;
		
		}else if (this->value("*", &crc)) { // Check for checksum
			// Checksum appended, check if it correct
			if ((uint8_t)crc == 0xff) {
				return GCODE_PACKET_READY;
			}else{
				return GCODE_PACKET_CRC_UNVALID;
			}
		}else{
			return GCODE_PACKET_CRC_MISSING;
		}
	}

	return GCODE_PACKET_NONE;
}

void GCode::send(char *command) { 
	this->send(command, this->useCRC);
}

void GCode::send(char *command, bool checksum)
{
	char buf[MAX_PACKET_SIZE + 8] = {'\0'};
	strncpy(buf, command, sizeof(buf) - 8);
	buf[sizeof(buf) - 8] = '\0';

	if (checksum) {
		strncat(buf, " *255", sizeof(buf) - strlen(buf) - 1);
	}

	// Always append a newline to indicate end of command
	strncat(buf, "\n", sizeof(buf) - strlen(buf) - 1);

	if( this->sendFunc != NULL )
		this->sendFunc(buf);
}
