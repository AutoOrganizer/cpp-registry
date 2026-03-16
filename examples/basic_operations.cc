#include <iostream>
#include <string>

#include <cpp-registry/Registry.h>

int main() {
	using namespace kare;

	RegistryKey demo = Registry::currentUser.createSubKey("Software\\cpp-registry-demo");

	demo.setValue("LaunchCount", 42);
	demo.setValue("User", "alice");

	int         count = demo.getValue("LaunchCount").dWordValue();
	std::string name  = demo.getValue("User").stringValue();

	std::cout << "LaunchCount=" << count << "\n";
	std::cout << "User=" << name.c_str() << "\n";

	demo.deleteValue("LaunchCount");
	Registry::deleteKey("HKCU\\Software\\cpp-registry-demo");
	return 0;
}
