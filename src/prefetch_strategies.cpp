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
#include "controller.h"
#include "document.h"
#include "render_internal.h"

namespace Render {
// Tools
namespace {
	void prefetch_next_n (const Request & context,
	                      const std::function<void(const Info &)> & launch_render, int n) {
		auto * current_page = context.current_page ();
		do {
			--n;
			current_page = current_page->next_page ();

			auto * render_page = page_for_role (current_page, context.role ());
			if (render_page != nullptr) {
				launch_render (Info{render_page, context.box_size ()});
			}
		} while (n > 0 && current_page != nullptr);
	}
	void prefetch_previous_n (const Request & context,
	                          const std::function<void(const Info &)> & launch_render, int n) {
		auto * current_page = context.current_page ();
		do {
			--n;
			current_page = current_page->previous_page ();

			auto * render_page = page_for_role (current_page, context.role ());
			if (render_page != nullptr) {
				launch_render (Info{render_page, context.box_size ()});
			}
		} while (n > 0 && current_page != nullptr);
	}
} // namespace

// No prefetch
class DisabledStrategy : public PrefetchStrategy {
public:
	DisabledStrategy () : PrefetchStrategy ("disabled") {}
	void prefetch (const Request &, const std::function<void(const Info &)> &) final {}
};

/* Reasonnable prefetch:
 * Always prefetch the next/prev page for every action.
 * When moving, prefetch the 5 next pages in the direction of movement for current page role.
 */
class DefaultStrategy : public PrefetchStrategy {
public:
	DefaultStrategy () : PrefetchStrategy ("default") {}

	void prefetch (const Request & context,
	               const std::function<void(const Info &)> & launch_render) final {
		bool has_directional_long_prefetch =
		    context.role () == ViewRole::CurrentPublic || context.role () == ViewRole::CurrentPresenter;

		if (has_directional_long_prefetch && context.cause () == RedrawCause::ForwardMove) {
			prefetch_next_n (context, launch_render, 5);
			prefetch_previous_n (context, launch_render, 1);
		} else if (has_directional_long_prefetch && context.cause () == RedrawCause::BackwardMove) {
			prefetch_next_n (context, launch_render, 1);
			prefetch_previous_n (context, launch_render, 5);
		} else {
			prefetch_next_n (context, launch_render, 1);
			prefetch_previous_n (context, launch_render, 1);
		}
	}
};

/* Listing and selection.
 *
 * PrefetchStrategy instances are created as global variables.
 * Only one of them will ever be used for the presentation, by the same thread.
 * Thus this system is ok for now.
 *
 * PrefetchStrategy structures have a non const prefetch function.
 * They might want to store and update an internal state.
 */
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
