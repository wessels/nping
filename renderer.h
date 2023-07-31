
#include <curses.h>
#include "site.h"

class renderer {
    public:
	renderer();
	~renderer();
	void render(const site *) const;
    private:
	WINDOW *theWindow;
};
