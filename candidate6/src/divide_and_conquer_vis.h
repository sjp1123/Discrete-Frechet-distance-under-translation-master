#pragma once

#include "frechet_under_translation_types.h"

#include <fstream>
#include <string>
#include <vector>

class DivideAndConquerVis
{
	using Level = std::vector<SearchBox>;
	using Length = double;
	static constexpr std::size_t MAX_LEVEL = std::numeric_limits<std::size_t>::max();

public:
	DivideAndConquerVis() = default;

	void add(SearchBox const& box, std::size_t level);
	void pop();
	void writeSvg(std::string const& filename, std::size_t max_level = MAX_LEVEL);
	void clear();

private:
	std::vector<Level> levels;

	Length const size = 1000.;
	Length const page_padding = 30.;

	void preprocessing();
	void writeHeader(std::ofstream& f);
	void writeLevels(std::ofstream& f, std::size_t max_level);
	void writeFooter(std::ofstream& f);

	Length yTransform(Length y);
};
