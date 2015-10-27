# Json
Simple Json file parser/writer writing in C++ for fun, just drop Json.hpp to your project and it will works.

# Usage
```C++
#include <iostream>
#include <string>
#include "Json.hpp"

int main()
{
	auto obj = Json::CreateJson<Json::Object>();
	(*obj)[Json::String("String")] = Json::CreateJson<Json::String>("Json String");
	(*obj)[Json::String("Number")] = Json::CreateJson<Json::Number>(1234);
	(*obj)[Json::String("Null")] = Json::CreateJson<Json::Null>();
	(*obj)[Json::String("Obj")] = Json::CreateJson<Json::Object>();
	auto array = (*obj)[Json::String("Array")] = Json::CreateJson<Json::Array>();

	// fill array
	auto arrayPtr = Json::ConvertJson<Json::Array>(array);
	for (auto i = 0; i < 10; ++i)
	{
		arrayPtr->push_back(Json::CreateJson<Json::Number>(i));
	}

	// write to file
	Json::FileWriter writer;
	writer.write("test.json", obj);

	try
	{
		// read
		Json::FileReader reader;
		auto obj = Json::ConvertJson<Json::Object>(reader.parseFile("test.json"));
		// or parse from std string
		// auto obj = Json::ConvertJson<Json::Object>(reader.parseFile("{}"));

		auto str = obj->get<Json::String>("String")->toSTDString();
		auto number = obj->get<Json::Number>("Number")->toLongLong();
		auto arrayPtr = obj->get<Json::Array>("Array");
		
		for (auto i = 0U; i < arrayPtr->count(); ++i)
		{
			auto item = Json::ConvertJson<Json::Number>((*arrayPtr)[i])->toLongLong();
		}
	}
	catch (Json::ParserException const &e)
	{
		std::cout << e.getMessage() << std::endl;
	}

	return EXIT_SUCCESS;
}
```
