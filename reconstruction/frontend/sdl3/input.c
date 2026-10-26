#include "input.h"

int tera_sdl_key(const SDL_KeyboardEvent *e)
{
    if(!e || e->type!=SDL_EVENT_KEY_DOWN || e->repeat ||
       (e->mod&(SDL_KMOD_CTRL|SDL_KMOD_ALT|SDL_KMOD_GUI))) return -1;
    if((e->mod&SDL_KMOD_NUM) && e->key>=SDLK_KP_1 && e->key<=SDLK_KP_0) return -1;
    switch(e->key) {
    case SDLK_UP:case SDLK_KP_8:return '8';
    case SDLK_DOWN:case SDLK_KP_2:return '2';
    case SDLK_LEFT:case SDLK_KP_4:return '4';
    case SDLK_RIGHT:case SDLK_KP_6:return '6';
    case SDLK_KP_0:return '0';case SDLK_KP_1:return '1';
    case SDLK_KP_3:return '3';case SDLK_KP_5:return '5';
    case SDLK_KP_7:return '7';case SDLK_KP_9:return '9';
    case SDLK_RETURN:case SDLK_KP_ENTER:return 13;
    case SDLK_BACKSPACE:return 8;case SDLK_ESCAPE:return 27;
    default:break;
    }
    /* Les caracteres imprimables arrivent par SDL_EVENT_TEXT_INPUT pour
     * respecter le layout (notamment chiffres AZERTY et # avec AltGr). */
    return -1;
}
