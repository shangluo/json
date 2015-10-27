# Json
Simple Json file parser/writer writing in C++ for fun, just drop Json.hpp to your project and it will works.

# Usage
```C++
#include  <iostream>
#include "Json.hpp"

int main()
{
	auto obj = Json::CreateJson<Json::Object>();
	(*obj)[Json::String("A")] = Json::CreateJson<Json::String>("456");
	(*obj)[Json::String("B")] = Json::CreateJson<Json::Number>(1234);
	(*obj)[Json::String("C")] = Json::CreateJson<Json::Null>();

	Json::FileWriter writer;
	writer.write("test.json", obj);

	try
	{
		Json::FileReader reader;
		auto obj2 = reader.parseFile("test.json");
		
		// or parse from std string
		// auto obj2 = reader.parse("{}");
	}
	catch (Json::ParserException const &e)
	{
		std::cout << e.getMessage() << std::endl;
	}

	return EXIT_SUCCESS;
}
```
