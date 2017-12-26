#include "catch.hpp"

#include "small_string.hpp"

#include <cstring>

TEST_CASE("basic c string manipulation", "[SmallString]") {
	auto s1 = gregjm::SmallString{ "ayy lmao" };

	REQUIRE(s1.size() == std::strlen("ayy lmao"));
	REQUIRE(s1.capacity() == 15);
	REQUIRE(s1 == "ayy lmao");

	s1 = "dank memes";

	REQUIRE(s1.size() == std::strlen("dank memes"));
	REQUIRE(s1.capacity() == 15);
	REQUIRE(s1 == "dank memes");

	s1 = "this is a pretty long string that won't be short";

	REQUIRE(s1.size() == std::strlen("this is a pretty long string that won't be short"));
	REQUIRE(s1.capacity() >= std::strlen("this is a pretty long string that won't be short"));
	REQUIRE(s1 == "this is a pretty long string that won't be short");

	s1.resize(4);

	REQUIRE(s1.size() == 4);
	REQUIRE(s1.capacity() >= 4);
	REQUIRE(s1 == "this");

	s1.resize(16);

	REQUIRE(s1.size() == 16);
	REQUIRE(s1.capacity() >= 16);

	s1 = "short";

	REQUIRE(s1.size() == std::strlen("short"));
	REQUIRE(s1.capacity() >= 16);
	REQUIRE(s1 == "short");

	s1.clear();

	REQUIRE(s1.size() == 0);
	REQUIRE(s1.capacity() == 15);
	REQUIRE(s1 == "");
}
