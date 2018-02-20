/* PDFTalk - PDF presentation tool
 * Copyright (C) 2016 - 2018 Francois Gindraud
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "document.h"
#include "render_internal.h"

namespace Render {
// No prefetch
class DisabledStrategy : public PrefetchStrategy {
public:
	DisabledStrategy () : PrefetchStrategy ("disabled") {}
	void prefetch (const Request &, const std::function<void(const Info &)> &) final {}
};

// Reasonnable prefetch
class DefaultStrategy : public PrefetchStrategy {
public:
	static int directed_movement_window (ViewRole role) {
		if (role == ViewRole::CurrentPublic || role == ViewRole::CurrentPresenter) {
			return 5;
		} else {
			return 1;
		}
	}
	static constexpr int default_window = 1;

	DefaultStrategy () : PrefetchStrategy ("default") {}

	void prefetch (const Request & context,
	               const std::function<void(const Info &)> & launch_render) final {}
};

// Listing and selection
namespace {
	DisabledStrategy disabled;
	DefaultStrategy defaulted;

	PrefetchStrategy * defined_strategies[] = {&disabled, &defaulted};
} // namespace

QStringList list_of_prefetch_strategy_names () {
	QStringList names;
	for (const auto * strategy : defined_strategies) {
		names << strategy->name ();
	}
	return names;
}

PrefetchStrategy * default_prefetch_strategy () {
	return &defaulted;
}
PrefetchStrategy * select_prefetch_strategy_by_name (const QString & name) {
	for (auto * strategy : defined_strategies) {
		if (strategy->name () == name.trimmed ()) {
			return strategy;
		}
	}
	return nullptr;
}

} // namespace Render
