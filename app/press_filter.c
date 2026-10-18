#include <stddef.h>
#include "press_filter.h"

uint8_t press_filter_events(const press_filter_in_t *in, event_t out[EVENT_COUNT])
{
    uint8_t k = 0U;                     /* events written so far */
    uint8_t both_pressed;               /* declared first: also builds as C90 */

    if ((in == NULL) || (out == NULL)) {
        return 0U;
    }

    /* Rule 2: both edges in the same pass, neither came first: drop both.
     * Checked here on its own, so it does not depend on how the driver sets held. */
    both_pressed = (uint8_t)(in->left_pressed && in->right_pressed);

    /* Rule 1: a press is dropped while the OTHER side is held. */
    if (in->left_pressed  && !in->right_held && !both_pressed) { out[k++] = EVENT_LEFT;   }
    if (in->right_pressed && !in->left_held  && !both_pressed) { out[k++] = EVENT_RIGHT;  }
    /* Rule 3: HAZARD is never in conflict. */
    if (in->hazard_pressed)                                    { out[k++] = EVENT_HAZARD; }

    return k;
}
