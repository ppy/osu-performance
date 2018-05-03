#pragma once

#define STREAMTOSTRING( x ) dynamic_cast< std::ostringstream & >( \
	(std::ostringstream{} << std::dec << x)).str()
