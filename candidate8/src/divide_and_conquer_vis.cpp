#include "divide_and_conquer_vis.h"

#include "defs.h"

void DivideAndConquerVis::add(SearchBox const& box, std::size_t level)
{
	if (level >= levels.size()) {
		levels.resize(level+1);
	}
	levels[level].push_back(box);
}

void DivideAndConquerVis::pop()
{
	assert(!levels.empty() && !levels.back().empty());
	levels.back().pop_back();
}

void DivideAndConquerVis::writeSvg(std::string const& filename, std::size_t max_level)
{
	std::ofstream f(filename);
	if (!f.is_open()) {
		ERROR("The filename passed to writeSvg cannot be opened.");
	}

	if (levels.empty()) { return; }

	preprocessing();

	f << std::fixed << std::setprecision(10);
	writeHeader(f);
	writeLevels(f, max_level);
	writeFooter(f);
	f.close();
}

void DivideAndConquerVis::clear()
{
	levels.clear();
}

void DivideAndConquerVis::preprocessing()
{
	// there should be a single box on the lowest level containing everything
	assert(levels[0].size() == 1);

	Length min_x = levels[0][0].min.x;
	Length min_y = levels[0][0].min.y;
	Length max_x = levels[0][0].max.x;
	Length max_y = levels[0][0].max.y;
	Length scaling = std::min(size/(max_x - min_x), size/(max_y - min_y));

	for (auto& level: levels) {
		for (auto& box: level) {
			box.min.x = (box.min.x - min_x)*scaling;
			box.min.y = (box.min.y - min_y)*scaling;
			box.max.x = (box.max.x - min_x)*scaling;
			box.max.y = (box.max.y - min_y)*scaling;
		}
	}
}

void DivideAndConquerVis::writeHeader(std::ofstream& f)
{
	f << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
	  << "<svg xmlns=\"http://www.w3.org/2000/svg\" "
	  << "xmlns:inkscape=\"http://www.inkscape.org/namespaces/inkscape\" "
	  << "viewBox=\"" << -page_padding << " " << -page_padding << " "
	  << size + 2*page_padding << " " << size + 2*page_padding << "\">\n"
	  << "<rect x=\"" << -page_padding << "\" y=\"" << -page_padding << "\" "
	  << "width=\"" << size + 2*page_padding << "\" height=\"" << size + 2*page_padding
	  << "\" fill=\"white\"/>\n";
}

void DivideAndConquerVis::writeLevels(std::ofstream& f, std::size_t max_level)
{
	std::string stroke_color = "black";

	Length stroke_width = 5.;
	int red = 0;

	for (std::size_t i = 0; i < std::min(levels.size(), max_level); ++i) {
		auto const& level = levels[i];
		red = (255*i)/levels.size();

		f << "<g id=\"" << i << "\" inkscape:label=\"" << i << "\" inkscape:groupmode=\"layer\">";
		for (auto const& box: level) {
			f << "<rect x=\"" << box.min.x
			  << "\" y=\"" << yTransform(box.max.y)
			  << "\" width=\"" << box.max.x - box.min.x
			  << "\" height=\"" << box.max.y - box.min.y
			  << "\" style=\"fill:none;stroke:rgb(" << red << ",0,0);stroke-width:" << stroke_width
			  << "\"/>\n";
		}
		f << "</g>\n";

		stroke_width /= 1.2;
	}
}

void DivideAndConquerVis::writeFooter(std::ofstream& f)
{
	f << "\n</svg>";
}

auto DivideAndConquerVis::yTransform(Length y) -> Length
{
	return size - y;
}
