#pragma once

// converts a well formed packet into a string in dest.
// returns the written bytes
// h is the packet to write
int send_request_type(int socket,Type tp);

