#pragma once

#pragma warning(push)

#pragma warning(disable : 4244)
#pragma warning(disable : 4267)

#include <crow.h>
#include "RateLimit/GlobalRateLimitMiddleware.hpp"
#pragma warning(pop)

using CrowApp = crow::App<GlobalRateLimitMiddleware>;