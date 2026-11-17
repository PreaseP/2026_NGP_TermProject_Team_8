#include "ServerApp.h"

#include <cstdio>
#include <cstdlib>
#include <memory>

int main()
{
	// [11.23] 세션과 큐를 포함하는 서버 앱은 힙에 둔다.
	auto app = std::make_unique<ServerApp>();
	if (!app->Init(SERVER_PORT)) {
		std::fprintf(stderr, "Server initialization failed (TCP port %u).\n",
			static_cast<unsigned int>(SERVER_PORT));
		return EXIT_FAILURE;
	}

	const int result = app->Run();
	app->Shutdown();
	if (result != EXIT_SUCCESS) {
		std::fputs("Server run failed.\n", stderr);
	}
	return result;
}
