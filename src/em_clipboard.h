#ifdef __EMSCRIPTEN__

// original here, MIT licensed
// https://github.com/Armchair-Software/emscripten-browser-clipboard/tree/main 

#include <emscripten.h>

#ifndef _EMCLIP_H_

typedef void (*emclip_paste_func)(const char*, void*);
typedef char const*(*emclip_copy_func)(void*);

void emclip_enable_hotkeys();

void emclip_set_paste_callback(emclip_paste_func callback, void *userdata);
void emclip_set_copy_callback(emclip_copy_func callback, void *userdata);
void emclip_copy_async(const char* );
#endif // _EMCLIP_H_

#ifndef _EMCLIP_C_
#ifdef EMCLIP_IMPLEMENTATION

void emclip_enable_hotkeys()
{
	EM_ASM({
		window.addEventListener('keydown', function(event){
			if (event.ctrlKey && event.key == 'c')    
				event.stopImmediatePropagation();
			if (event.ctrlKey && event.key == 'v')    
				event.stopImmediatePropagation();
		}, true);
	});
}

EM_JS(void, emclip_set_paste_callback, (emclip_paste_func callback, void *userdata),
{
	document.addEventListener('paste', (event) => {
		Module.ccall(
			'emclip_internal_paste_handler', 
			'number', 
			['string', 'number', 'number'], 
			[event.clipboardData.getData('text/plain'), 
				callback, 
				userdata]);
	});
});

EM_JS(void, emclip_set_copy_callback, (emclip_copy_func callback, void *userdata),
{
	document.addEventListener('copy', (event) => {
		const content_ptr = Module.ccall(
			'emclip_internal_copy_handler', 
			'number', 
			['number', 'number'], 
			[callback, userdata]);
		event.clipboardData.setData('text/plain', UTF8ToString(content_ptr));
		event.preventDefault();
	});
});

EM_JS(void, emclip_copy_async, (char const *content_ptr), 
{
	navigator.clipboard.writeText(UTF8ToString(content_ptr));
});

EMSCRIPTEN_KEEPALIVE 
int emclip_internal_paste_handler(
		char const *paste_data, 
		emclip_paste_func callback, 
		void *userdata) 
{
	callback(paste_data, userdata);
	return 1;
}

EMSCRIPTEN_KEEPALIVE 
char const* emclip_internal_copy_handler(
		emclip_copy_func callback, 
		void *userdata) 
{
	return callback(userdata);
}

#endif // EMCLIP_IMPLEMENTATION

#endif // _EMCLIP_C_

#endif // __EMSCRIPTEN__