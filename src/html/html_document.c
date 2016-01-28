/*
 * This file is part of libdom.
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2009 Bo Yang <struggleyb.nku@gmail.com>
 */

#include <assert.h>
#include <stdlib.h>

#include <dom/html/html_elements.h>

#include "html/html_document.h"
#include "html/html_element.h"
#include "html/html_collection.h"
#include "html/html_html_element.h"
#include "html/html_head_element.h"
#include "html/html_body_element.h"
#include "html/html_base_element.h"
#include "html/html_div_element.h"
#include "html/html_link_element.h"
#include "html/html_title_element.h"
#include "html/html_meta_element.h"
#include "html/html_form_element.h"
#include "html/html_button_element.h"
#include "html/html_input_element.h"
#include "html/html_text_area_element.h"
#include "html/html_opt_group_element.h"
#include "html/html_option_element.h"
#include "html/html_select_element.h"
#include "html/html_hr_element.h"
#include "html/html_dlist_element.h"
#include "html/html_directory_element.h"
#include "html/html_menu_element.h"
#include "html/html_fieldset_element.h"
#include "html/html_legend_element.h"
#include "html/html_paragraph_element.h"
#include "html/html_heading_element.h"
#include "html/html_quote_element.h"
#include "html/html_pre_element.h"
#include "html/html_br_element.h"
#include "html/html_label_element.h"
#include "html/html_ulist_element.h"
#include "html/html_olist_element.h"
#include "html/html_li_element.h"
#include "html/html_font_element.h"
#include "html/html_mod_element.h"
#include "html/html_anchor_element.h"
#include "html/html_basefont_element.h"
#include "html/html_image_element.h"
#include "html/html_object_element.h"
#include "html/html_param_element.h"
#include "html/html_applet_element.h"
#include "html/html_map_element.h"
#include "html/html_area_element.h"
#include "html/html_script_element.h"
#include "html/html_tablecaption_element.h"
#include "html/html_tablecell_element.h"
#include "html/html_tablecol_element.h"
#include "html/html_tablesection_element.h"
#include "html/html_table_element.h"
#include "html/html_tablerow_element.h"
#include "html/html_style_element.h"
#include "html/html_frameset_element.h"
#include "html/html_frame_element.h"
#include "html/html_iframe_element.h"
#include "html/html_isindex_element.h"

#include "core/attr.h"
#include "core/string.h"
#include "utils/namespace.h"
#include "utils/utils.h"

static struct dom_html_document_vtable html_document_vtable = {
	{
		{
			{
				DOM_NODE_EVENT_TARGET_VTABLE
			},
			DOM_NODE_VTABLE_DOCUMENT,
		},
		DOM_DOCUMENT_VTABLE_HTML
	},
	DOM_HTML_DOCUMENT_VTABLE
};

static struct dom_node_protect_vtable html_document_protect_vtable = {
	DOM_HTML_DOCUMENT_PROTECT_VTABLE
};

/* Create a HTMLDocument */
dom_exception _dom_html_document_create(
		dom_events_default_action_fetcher daf,
		void *daf_ctx,
		dom_html_document **doc)
{
	dom_exception error;
	dom_html_document *result;

	result = malloc(sizeof(dom_html_document));
	if (result == NULL)
		return DOM_NO_MEM_ERR;

	result->base.base.base.vtable = &html_document_vtable;
	result->base.base.vtable = &html_document_protect_vtable;
	
	error = _dom_html_document_initialise(result, daf, daf_ctx);
	if (error != DOM_NO_ERR) {
		free(result);
		return error;
	}

	*doc = result;
	return DOM_NO_ERR;
}

/* Initialise a HTMLDocument */
dom_exception _dom_html_document_initialise(dom_html_document *doc,
		dom_events_default_action_fetcher daf,
		void *daf_ctx)
{
	dom_exception error;
	int sidx;

	error = _dom_document_initialise(&doc->base, daf, daf_ctx);
	if (error != DOM_NO_ERR)
		return error;

	doc->title = NULL;
	doc->referrer = NULL;
	doc->domain = NULL;
	doc->url = NULL;
	doc->cookie = NULL;
	doc->body = NULL;

	doc->memoised = calloc(sizeof(dom_string *), hds_COUNT);
	if (doc->memoised == NULL) {
		error = DOM_NO_MEM_ERR;
		goto out;
	}
	doc->elements = calloc(sizeof(dom_string *),
			DOM_HTML_ELEMENT_TYPE__COUNT);
	if (doc->elements == NULL) {
		error = DOM_NO_MEM_ERR;
		goto out;
	}

#define HTML_DOCUMENT_STRINGS_ACTION(attr,str)				\
	error = dom_string_create_interned((const uint8_t *) #str,	\
			SLEN(#str), &doc->memoised[hds_##attr]);	\
	if (error != DOM_NO_ERR) {					\
		goto out;						\
	}

#include "html_document_strings.h"
#undef HTML_DOCUMENT_STRINGS_ACTION

#define DOM_HTML_ELEMENT_STRINGS_ENTRY(tag)				\
	error = dom_string_create_interned((const uint8_t *) #tag,	\
			SLEN(#tag),					\
			&doc->elements[DOM_HTML_ELEMENT_TYPE_##tag]);	\
	if (error != DOM_NO_ERR) {					\
		goto out;						\
	}

#include <dom/html/html_elements.h>
#undef DOM_HTML_ELEMENT_STRINGS_ENTRY

out:
	if (error != DOM_NO_ERR) {
		if (doc->memoised != NULL) {
			for(sidx = 0; sidx < hds_COUNT; ++sidx) {
				if (doc->memoised[sidx] != NULL) {
					dom_string_unref(doc->memoised[sidx]);
				}
			}
			free(doc->memoised);
			doc->memoised = NULL;
		}
		if (doc->elements != NULL) {
			for(sidx = 0; sidx < DOM_HTML_ELEMENT_TYPE__COUNT;
					++sidx) {
				if (doc->elements[sidx] != NULL) {
					dom_string_unref(doc->elements[sidx]);
				}
			}
			free(doc->elements);
			doc->elements = NULL;
		}
	}
	return error;
}

/* Finalise a HTMLDocument */
bool _dom_html_document_finalise(dom_html_document *doc)
{
	int sidx;
	
	if (doc->cookie != NULL)
		dom_string_unref(doc->cookie);
	if (doc->url != NULL)
		dom_string_unref(doc->url);
	if (doc->domain != NULL)
		dom_string_unref(doc->domain);
	if (doc->referrer != NULL)
		dom_string_unref(doc->referrer);
	if (doc->title != NULL)
		dom_string_unref(doc->title);
	
	if (doc->memoised != NULL) {
		for(sidx = 0; sidx < hds_COUNT; ++sidx) {
			if (doc->memoised[sidx] != NULL) {
				dom_string_unref(doc->memoised[sidx]);
			}
		}
		free(doc->memoised);
		doc->memoised = NULL;
	}
	
	if (doc->elements != NULL) {
		for(sidx = 0; sidx < DOM_HTML_ELEMENT_TYPE__COUNT; ++sidx) {
			if (doc->elements[sidx] != NULL) {
				dom_string_unref(doc->elements[sidx]);
			}
		}
		free(doc->elements);
		doc->elements = NULL;
	}

	return _dom_document_finalise(&doc->base);
}

/* Destroy a HTMLDocument */
void _dom_html_document_destroy(dom_node_internal *node)
{
	dom_html_document *doc = (dom_html_document *) node;

	if (_dom_html_document_finalise(doc) == true)
		free(doc);
}

dom_exception _dom_html_document_copy(dom_node_internal *old,
		dom_node_internal **copy)
{
	UNUSED(old);
	UNUSED(copy);

	return DOM_NOT_SUPPORTED_ERR;
}

/**
 * Get html element type from an uppercased tag name string
 *
 * \param html            The html document that the html element belongs to
 * \param tag_name_upper  Upper cased string containing tag name
 */
static inline dom_html_element_type _dom_html_document_get_element_type(
		dom_html_document *html, dom_string *tag_name_upper)
{
	size_t len = dom_string_byte_length(tag_name_upper);

	switch (len) {
	case 1:
		if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_P])) {
			return  DOM_HTML_ELEMENT_TYPE_P;
		} else if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_A])) {
			return  DOM_HTML_ELEMENT_TYPE_A;
		} else if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_Q])) {
			return  DOM_HTML_ELEMENT_TYPE_Q;
		}
		break;
	case 2:
		if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_H1])) {
			return  DOM_HTML_ELEMENT_TYPE_H1;
		} else if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_H2])) {
			return  DOM_HTML_ELEMENT_TYPE_H2;
		} else if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_H3])) {
			return  DOM_HTML_ELEMENT_TYPE_H3;
		} else if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_H4])) {
			return  DOM_HTML_ELEMENT_TYPE_H4;
		} else if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_H5])) {
			return  DOM_HTML_ELEMENT_TYPE_H5;
		} else if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_H6])) {
			return  DOM_HTML_ELEMENT_TYPE_H6;
		} else if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_HR])) {
			return  DOM_HTML_ELEMENT_TYPE_HR;
		} else if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_DL])) {
			return  DOM_HTML_ELEMENT_TYPE_DL;
		} else if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_UL])) {
			return  DOM_HTML_ELEMENT_TYPE_UL;
		} else if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_OL])) {
			return  DOM_HTML_ELEMENT_TYPE_OL;
		} else if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_LI])) {
			return  DOM_HTML_ELEMENT_TYPE_LI;
		} else if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_TD])) {
			return  DOM_HTML_ELEMENT_TYPE_TD;
		} else if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_TH])) {
			return  DOM_HTML_ELEMENT_TYPE_TH;
		} else if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_TR])) {
			return  DOM_HTML_ELEMENT_TYPE_TR;
		} else if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_BR])) {
			return  DOM_HTML_ELEMENT_TYPE_BR;
		}
		break;
	case 3:
		if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_DIV])) {
			return  DOM_HTML_ELEMENT_TYPE_DIV;
		} else if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_PRE])) {
			return  DOM_HTML_ELEMENT_TYPE_PRE;
		} else if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_DEL])) {
			return  DOM_HTML_ELEMENT_TYPE_DEL;
		} else if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_INS])) {
			return  DOM_HTML_ELEMENT_TYPE_INS;
		} else if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_IMG])) {
			return  DOM_HTML_ELEMENT_TYPE_IMG;
		} else if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_COL])) {
			return  DOM_HTML_ELEMENT_TYPE_COL;
		} else if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_MAP])) {
			return  DOM_HTML_ELEMENT_TYPE_MAP;
		} else if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_DIR])) {
			return  DOM_HTML_ELEMENT_TYPE_DIR;
		}
		break;
	case 4:
		if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_HTML])) {
			return  DOM_HTML_ELEMENT_TYPE_HTML;
		} else if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_HEAD])) {
			return  DOM_HTML_ELEMENT_TYPE_HEAD;
		} else if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_META])) {
			return  DOM_HTML_ELEMENT_TYPE_META;
		} else if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_BASE])) {
			return  DOM_HTML_ELEMENT_TYPE_BASE;
		} else if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_BODY])) {
			return  DOM_HTML_ELEMENT_TYPE_BODY;
		} else if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_FORM])) {
			return  DOM_HTML_ELEMENT_TYPE_FORM;
		} else if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_LINK])) {
			return  DOM_HTML_ELEMENT_TYPE_LINK;
		} else if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_MENU])) {
			return  DOM_HTML_ELEMENT_TYPE_MENU;
		} else if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_FONT])) {
			return  DOM_HTML_ELEMENT_TYPE_FONT;
		} else if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_AREA])) {
			return  DOM_HTML_ELEMENT_TYPE_AREA;
		}
		break;
	case 5:
		if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_TITLE])) {
			return  DOM_HTML_ELEMENT_TYPE_TITLE;
		} else if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_INPUT])) {
			return  DOM_HTML_ELEMENT_TYPE_INPUT;
		} else if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_FRAME])) {
			return  DOM_HTML_ELEMENT_TYPE_FRAME;
		} else if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_THEAD])) {
			return  DOM_HTML_ELEMENT_TYPE_THEAD;
		} else if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_TBODY])) {
			return  DOM_HTML_ELEMENT_TYPE_TBODY;
		} else if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_TFOOT])) {
			return  DOM_HTML_ELEMENT_TYPE_TFOOT;
		} else if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_TABLE])) {
			return  DOM_HTML_ELEMENT_TYPE_TABLE;
		} else if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_STYLE])) {
			return  DOM_HTML_ELEMENT_TYPE_STYLE;
		} else if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_LABEL])) {
			return  DOM_HTML_ELEMENT_TYPE_LABEL;
		} else if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_PARAM])) {
			return  DOM_HTML_ELEMENT_TYPE_PARAM;
		}
		break;
	case 6:
		if (dom_string_isequal(tag_name_upper,html->elements[
				DOM_HTML_ELEMENT_TYPE_BUTTON])) {
			return  DOM_HTML_ELEMENT_TYPE_BUTTON;
		} else if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_OPTION])) {
			return  DOM_HTML_ELEMENT_TYPE_OPTION;
		} else if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_SCRIPT])) {
			return  DOM_HTML_ELEMENT_TYPE_SCRIPT;
		} else if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_IFRAME])) {
			return  DOM_HTML_ELEMENT_TYPE_IFRAME;
		} else if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_LEGEND])) {
			return  DOM_HTML_ELEMENT_TYPE_LEGEND;
		} else if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_SELECT])) {
			return  DOM_HTML_ELEMENT_TYPE_SELECT;
		} else if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_OBJECT])) {
			return  DOM_HTML_ELEMENT_TYPE_OBJECT;
		} else if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_APPLET])) {
			return  DOM_HTML_ELEMENT_TYPE_APPLET;
		}
		break;
	case 7:
		if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_CAPTION])) {
			return  DOM_HTML_ELEMENT_TYPE_CAPTION;
		} else if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_ISINDEX])) {
			return  DOM_HTML_ELEMENT_TYPE_ISINDEX;
		}
		break;
	case 8:
		if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_TEXTAREA])) {
			return  DOM_HTML_ELEMENT_TYPE_TEXTAREA;
		} else if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_OPTGROUP])) {
			return  DOM_HTML_ELEMENT_TYPE_OPTGROUP;
		} else if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_BASEFONT])) {
			return  DOM_HTML_ELEMENT_TYPE_BASEFONT;
		} else if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_FIELDSET])) {
			return  DOM_HTML_ELEMENT_TYPE_FIELDSET;
		} else if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_COLGROUP])) {
			return  DOM_HTML_ELEMENT_TYPE_COLGROUP;
		} else if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_FRAMESET])) {
			return  DOM_HTML_ELEMENT_TYPE_FRAMESET;
		}
		break;
	case 10:
		if (dom_string_isequal(tag_name_upper, html->elements[
				DOM_HTML_ELEMENT_TYPE_BLOCKQUOTE])) {
			return  DOM_HTML_ELEMENT_TYPE_BLOCKQUOTE;
		}
		break;
	}

	return DOM_HTML_ELEMENT_TYPE__UNKNOWN;
}

/* Overloaded methods inherited from super class */

/** Internal method to support both kinds of create method */
static dom_exception
_dom_html_document_create_element_internal(
		dom_html_document *html,
		dom_string *in_tag_name,
		dom_string *namespace,
		dom_string *prefix,
		dom_html_element **result)
{
	dom_exception exc;
	dom_string *tag_name;
	dom_html_element_type type;

	exc = dom_string_toupper(in_tag_name, true, &tag_name);
	if (exc != DOM_NO_ERR)
		return exc;

	type = _dom_html_document_get_element_type(html, tag_name);

	switch(type) {
	case DOM_HTML_ELEMENT_TYPE__COUNT:
		assert(type != DOM_HTML_ELEMENT_TYPE__COUNT);
		break;
	case DOM_HTML_ELEMENT_TYPE_HTML:
		exc = _dom_html_html_element_create(html, namespace, prefix,
				(dom_html_html_element **) result);
		break;
	case DOM_HTML_ELEMENT_TYPE_HEAD:
		exc = _dom_html_head_element_create(html, namespace, prefix,
				(dom_html_head_element **) result);
		break;
	case DOM_HTML_ELEMENT_TYPE_META:
		exc = _dom_html_meta_element_create(html, namespace, prefix,
				(dom_html_meta_element **) result);
		break;
	case DOM_HTML_ELEMENT_TYPE_BASE:
		exc = _dom_html_base_element_create(html, namespace, prefix,
				(dom_html_base_element **) result);
		break;
	case DOM_HTML_ELEMENT_TYPE_TITLE:
		exc = _dom_html_title_element_create(html, namespace, prefix,
				(dom_html_title_element **) result);
		break;
	case DOM_HTML_ELEMENT_TYPE_BODY:
		exc = _dom_html_body_element_create(html, namespace, prefix,
				(dom_html_body_element **) result);
		break;
	case DOM_HTML_ELEMENT_TYPE_DIV:
		exc = _dom_html_div_element_create(html, namespace, prefix,
				(dom_html_div_element **) result);
		break;
	case DOM_HTML_ELEMENT_TYPE_FORM:
		exc = _dom_html_form_element_create(html, namespace, prefix,
				(dom_html_form_element **) result);
		break;
	case DOM_HTML_ELEMENT_TYPE_LINK:
		exc = _dom_html_link_element_create(html, namespace, prefix,
				(dom_html_link_element **) result);
		break;
	case DOM_HTML_ELEMENT_TYPE_BUTTON:
		exc = _dom_html_button_element_create(html, namespace, prefix,
				(dom_html_button_element **) result);
		break;
	case DOM_HTML_ELEMENT_TYPE_INPUT:
		exc = _dom_html_input_element_create(html, namespace, prefix,
				(dom_html_input_element **) result);
		break;
	case DOM_HTML_ELEMENT_TYPE_TEXTAREA:
		exc = _dom_html_text_area_element_create(html, namespace,
				prefix, (dom_html_text_area_element **) result);
		break;
	case DOM_HTML_ELEMENT_TYPE_OPTGROUP:
		exc = _dom_html_opt_group_element_create(html, namespace,
				prefix, (dom_html_opt_group_element **) result);
		break;
	case DOM_HTML_ELEMENT_TYPE_OPTION:
		exc = _dom_html_option_element_create(html, namespace, prefix,
				(dom_html_option_element **) result);
		break;
	case DOM_HTML_ELEMENT_TYPE_SELECT:
		exc = _dom_html_select_element_create(html, namespace, prefix,
				(dom_html_select_element **) result);
		break;
	case DOM_HTML_ELEMENT_TYPE_HR:
		exc = _dom_html_hr_element_create(html, namespace, prefix,
				(dom_html_hr_element **) result);
		break;
	case DOM_HTML_ELEMENT_TYPE_DL:
		exc = _dom_html_dlist_element_create(html, namespace, prefix,
				(dom_html_dlist_element **) result);
		break;
	case DOM_HTML_ELEMENT_TYPE_DIR:
		exc = _dom_html_directory_element_create(html, namespace,
				prefix, (dom_html_directory_element **) result);
		break;
	case DOM_HTML_ELEMENT_TYPE_MENU:
		exc = _dom_html_menu_element_create(html, namespace, prefix,
				(dom_html_menu_element **) result);
		break;
	case DOM_HTML_ELEMENT_TYPE_FIELDSET:
		exc = _dom_html_field_set_element_create(html, namespace,
				prefix, (dom_html_field_set_element **) result);
		break;
	case DOM_HTML_ELEMENT_TYPE_LEGEND:
		exc = _dom_html_legend_element_create(html, namespace, prefix,
				(dom_html_legend_element **) result);
		break;
	case DOM_HTML_ELEMENT_TYPE_P:
		exc = _dom_html_paragraph_element_create(html, namespace,
				prefix, (dom_html_paragraph_element **) result);
		break;
	case DOM_HTML_ELEMENT_TYPE_H1:
	case DOM_HTML_ELEMENT_TYPE_H2:
	case DOM_HTML_ELEMENT_TYPE_H3:
	case DOM_HTML_ELEMENT_TYPE_H4:
	case DOM_HTML_ELEMENT_TYPE_H5:
	case DOM_HTML_ELEMENT_TYPE_H6:
		exc = _dom_html_heading_element_create(html, tag_name,
				namespace, prefix,
				(dom_html_heading_element **) result);
		break;
	case DOM_HTML_ELEMENT_TYPE_BLOCKQUOTE:
	case DOM_HTML_ELEMENT_TYPE_Q:
		exc = _dom_html_quote_element_create(html, tag_name,
				namespace, prefix,
				(dom_html_quote_element **) result);
		break;
	case DOM_HTML_ELEMENT_TYPE_PRE:
		exc = _dom_html_pre_element_create(html, namespace, prefix,
				(dom_html_pre_element **) result);
		break;
	case DOM_HTML_ELEMENT_TYPE_BR:
		exc = _dom_html_br_element_create(html, namespace, prefix,
				(dom_html_br_element **) result);
		break;
	case DOM_HTML_ELEMENT_TYPE_LABEL:
		exc = _dom_html_label_element_create(html, namespace, prefix,
				(dom_html_label_element **) result);
		break;
	case DOM_HTML_ELEMENT_TYPE_UL:
		exc = _dom_html_u_list_element_create(html, namespace, prefix,
				(dom_html_u_list_element **) result);
		break;
	case DOM_HTML_ELEMENT_TYPE_OL:
		exc = _dom_html_olist_element_create(html, namespace, prefix,
				(dom_html_olist_element **) result);
		break;
	case DOM_HTML_ELEMENT_TYPE_LI:
		exc = _dom_html_li_element_create(html, namespace, prefix,
				(dom_html_li_element **) result);
		break;
	case DOM_HTML_ELEMENT_TYPE_FONT:
		exc = _dom_html_font_element_create(html, namespace, prefix,
				(dom_html_font_element **) result);
		break;
	case DOM_HTML_ELEMENT_TYPE_DEL:
	case DOM_HTML_ELEMENT_TYPE_INS:
		exc = _dom_html_mod_element_create(html, tag_name, namespace, 
				prefix, (dom_html_mod_element **) result);
		break;
	case DOM_HTML_ELEMENT_TYPE_A:
		exc = _dom_html_anchor_element_create(html, namespace, prefix,
				(dom_html_anchor_element **) result);
		break;
	case DOM_HTML_ELEMENT_TYPE_BASEFONT:
		exc = _dom_html_base_font_element_create(html,
				namespace, prefix,
				(dom_html_base_font_element **) result);
		break;
	case DOM_HTML_ELEMENT_TYPE_IMG:
		exc = _dom_html_image_element_create(html, namespace, prefix,
				(dom_html_image_element **) result);
		break;
	case DOM_HTML_ELEMENT_TYPE_OBJECT:
		exc = _dom_html_object_element_create(html, namespace, prefix,
				(dom_html_object_element **) result);
		break;
	case DOM_HTML_ELEMENT_TYPE_PARAM:
		exc = _dom_html_param_element_create(html, namespace, prefix,
				(dom_html_param_element **) result);
		break;
	case DOM_HTML_ELEMENT_TYPE_APPLET:
		exc = _dom_html_applet_element_create(html, namespace, prefix,
				(dom_html_applet_element **) result);
		break;
	case DOM_HTML_ELEMENT_TYPE_MAP:
		exc = _dom_html_map_element_create(html, namespace, prefix,
				(dom_html_map_element **) result);
		break;
	case DOM_HTML_ELEMENT_TYPE_AREA:
		exc = _dom_html_area_element_create(html, namespace, prefix,
				(dom_html_area_element **) result);
		break;
	case DOM_HTML_ELEMENT_TYPE_SCRIPT:
		exc = _dom_html_script_element_create(html, namespace, prefix,
				(dom_html_script_element **) result);
		break;
	case DOM_HTML_ELEMENT_TYPE_CAPTION:
		exc = _dom_html_table_caption_element_create(html,
				namespace, prefix,
				(dom_html_table_caption_element **) result);
		break;
	case DOM_HTML_ELEMENT_TYPE_TD:
	case DOM_HTML_ELEMENT_TYPE_TH:
		exc = _dom_html_table_cell_element_create(html, tag_name,
				namespace, prefix,
				(dom_html_table_cell_element **) result);
		break;
	case DOM_HTML_ELEMENT_TYPE_COL:
	case DOM_HTML_ELEMENT_TYPE_COLGROUP:
		exc = _dom_html_table_col_element_create(html, tag_name,
				namespace, prefix,
				(dom_html_table_col_element **) result);
		break;
	case DOM_HTML_ELEMENT_TYPE_THEAD:
	case DOM_HTML_ELEMENT_TYPE_TBODY:
	case DOM_HTML_ELEMENT_TYPE_TFOOT:
		exc = _dom_html_table_section_element_create(html, tag_name,
				namespace, prefix,
				(dom_html_table_section_element **) result);
		break;
	case DOM_HTML_ELEMENT_TYPE_TABLE:
		exc = _dom_html_table_element_create(html, namespace, prefix,
				(dom_html_table_element **) result);
		break;
	case DOM_HTML_ELEMENT_TYPE_TR:
		exc = _dom_html_table_row_element_create(html, namespace,
				prefix, (dom_html_table_row_element **) result);
		break;
	case DOM_HTML_ELEMENT_TYPE_STYLE:
		exc = _dom_html_style_element_create(html,
				(dom_html_style_element **) result);
		break;
	case DOM_HTML_ELEMENT_TYPE_FRAMESET:
		exc = _dom_html_frame_set_element_create(html, namespace,
				prefix, (dom_html_frame_set_element **) result);
		break;
	case DOM_HTML_ELEMENT_TYPE_FRAME:
		exc = _dom_html_frame_element_create(html, namespace, prefix,
				(dom_html_frame_element **) result);
		break;
	case DOM_HTML_ELEMENT_TYPE_IFRAME:
		exc = _dom_html_iframe_element_create(html, namespace, prefix,
				(dom_html_iframe_element **) result);
		break;
	case DOM_HTML_ELEMENT_TYPE_ISINDEX:
		exc = _dom_html_isindex_element_create(html, namespace, prefix,
				(dom_html_isindex_element **) result);
		break;
	case DOM_HTML_ELEMENT_TYPE__UNKNOWN:
		exc =  _dom_html_element_create(html, tag_name, namespace,
						prefix, result);
		break;
	}

	dom_string_unref(tag_name);

	return exc;
}

dom_exception _dom_html_document_create_element(dom_document *doc,
		dom_string *tag_name, dom_element **result)
{
	dom_html_document *html = (dom_html_document *) doc;

	return _dom_html_document_create_element_internal(html,
			tag_name, NULL, NULL,
			(dom_html_element **)result);
}

dom_exception _dom_html_document_create_element_ns(dom_document *doc,
		dom_string *namespace, dom_string *qname,
		dom_element **result)
{
	dom_html_document *html = (dom_html_document *) doc;
	dom_string *prefix, *localname;
	dom_exception err;

	/* Divide QName into prefix/localname pair */
	err = _dom_namespace_split_qname(qname, &prefix, &localname);
	if (err != DOM_NO_ERR) {
		return err;
	}

	/* Attempt to create element */
	err = _dom_html_document_create_element_internal(html, localname,
			namespace, prefix, (dom_html_element **)result);

	/* Tidy up */
	dom_string_unref(localname);

	if (prefix != NULL) {
		dom_string_unref(prefix);
	}

	return err;
}

/**
 * Create an attribute
 *
 * \param doc     The document owning the attribute
 * \param name    The name of the attribute
 * \param result  Pointer to location to receive result
 * \return DOM_NO_ERR                on success,
 *
 * The constructed attribute will always be classified as 'specified'.
 *
 * The returned node will have its reference count increased. It is
 * the responsibility of the caller to unref the node once it has
 * finished with it.
 */
dom_exception _dom_html_document_create_attribute(dom_document *doc,
		dom_string *name, dom_attr **result)
{
	return _dom_attr_create(doc, name, NULL, NULL, true, result);
}

/**
 * Create an attribute from the qualified name and namespace URI
 *
 * \param doc        The document owning the attribute
 * \param namespace  The namespace URI to use
 * \param qname      The qualified name of the attribute
 * \param result     Pointer to location to receive result
 * \return DOM_NO_ERR                on success,
 *         DOM_NAMESPACE_ERR         if ::qname is malformed, or it has a
 *                                   prefix and ::namespace is NULL, or
 *                                   ::qname has a prefix "xml" and
 *                                   ::namespace is not
 *                                   "http://www.w3.org/XML/1998/namespace",
 *                                   or ::qname has a prefix "xmlns" and
 *                                   ::namespace is not
 *                                   "http://www.w3.org/2000/xmlns", or
 *                                   ::namespace is
 *                                   "http://www.w3.org/2000/xmlns" and
 *                                   ::qname is not (or is not prefixed by)
 *                                   "xmlns",
 *         DOM_NOT_SUPPORTED_ERR     if ::doc does not support the "XML"
 *                                   feature.
 *
 * The returned node will have its reference count increased. It is
 * the responsibility of the caller to unref the node once it has
 * finished with it.
 */
dom_exception _dom_html_document_create_attribute_ns(dom_document *doc,
		dom_string *namespace, dom_string *qname,
		dom_attr **result)
{
	dom_string *prefix, *localname;
	dom_exception err;

	/* Divide QName into prefix/localname pair */
	err = _dom_namespace_split_qname(qname, &prefix, &localname);
	if (err != DOM_NO_ERR) {
		return err;
	}

	/* Attempt to create attribute */
	err = _dom_attr_create(doc, localname, namespace, prefix, true, result);

	/* Tidy up */
	if (localname != NULL) {
		dom_string_unref(localname);
	}

	if (prefix != NULL) {
		dom_string_unref(prefix);
	}

	return err;
}

/**
 * Retrieve a list of all elements with a given tag name
 *
 * \param doc      The document to search in
 * \param tagname  The tag name to search for ("*" for all)
 * \param result   Pointer to location to receive result
 * \return DOM_NO_ERR.
 *
 * The returned list will have its reference count increased. It is
 * the responsibility of the caller to unref the list once it has
 * finished with it.
 */
dom_exception _dom_html_document_get_elements_by_tag_name(dom_document *doc,
		dom_string *tagname, dom_nodelist **result)
{
	return _dom_document_get_nodelist(doc, DOM_NODELIST_BY_NAME_CASELESS,
			(dom_node_internal *) doc,  tagname, NULL, NULL, 
			result);
}

/**
 * Retrieve a list of all elements with a given local name and namespace URI
 *
 * \param doc        The document to search in
 * \param namespace  The namespace URI
 * \param localname  The local name
 * \param result     Pointer to location to receive result
 * \return DOM_NO_ERR on success, appropriate dom_exception on failure.
 *
 * The returned list will have its reference count increased. It is
 * the responsibility of the caller to unref the list once it has
 * finished with it.
 */
dom_exception _dom_html_document_get_elements_by_tag_name_ns(
		dom_document *doc, dom_string *namespace,
		dom_string *localname, dom_nodelist **result)
{
	return _dom_document_get_nodelist(doc, DOM_NODELIST_BY_NAMESPACE_CASELESS,
			(dom_node_internal *) doc, NULL, namespace, localname, 
			result);
}

/*-----------------------------------------------------------------------*/
/* The DOM spec public API */

/**
 * Get the title of this HTMLDocument 
 * \param doc    The document object
 * \param title  The reutrned title string
 * \return DOM_NO_ERR on success, appropriated dom_exception on failure.
 *
 * @note: this method find a title for the document as following:
 * 1. If there is a title in the document object set by 
 *    dom_html_document_set_title, then use it;
 * 2. If there is no such one, find the <title> element and use its text
 *    as the returned title.
 */
dom_exception _dom_html_document_get_title(dom_html_document *doc,
		dom_string **title)
{
	dom_exception exc = DOM_NO_ERR;
	*title = NULL;
	
	if (doc->title != NULL) {
		*title = dom_string_ref(doc->title);
	} else {
		dom_element *node;
		dom_nodelist *nodes;
		uint32_t len;
		
		exc = dom_document_get_elements_by_tag_name(doc,
				doc->elements[DOM_HTML_ELEMENT_TYPE_TITLE],
				&nodes);
		if (exc != DOM_NO_ERR) {
			return exc;
		}
		
		exc = dom_nodelist_get_length(nodes, &len);
		if (exc != DOM_NO_ERR) {
			dom_nodelist_unref(nodes);
			return exc;
		}
		
		if (len == 0) {
			dom_nodelist_unref(nodes);
			return DOM_NO_ERR;
		}
		
		exc = dom_nodelist_item(nodes, 0, (void *) &node);
		dom_nodelist_unref(nodes);
		if (exc != DOM_NO_ERR) {
			return exc;
		}
		
		exc = dom_node_get_text_content(node, title);
		dom_node_unref(node);
	}

	return exc;
}

dom_exception _dom_html_document_set_title(dom_html_document *doc,
		dom_string *title)
{
	if (doc->title != NULL)
		dom_string_unref(doc->title);

	doc->title = dom_string_ref(title);

	return DOM_NO_ERR;
}

dom_exception _dom_html_document_get_referrer(dom_html_document *doc,
		dom_string **referrer)
{
	*referrer = dom_string_ref(doc->referrer);

	return DOM_NO_ERR;
}

dom_exception _dom_html_document_get_domain(dom_html_document *doc,
		dom_string **domain)
{
	*domain = dom_string_ref(doc->domain);

	return DOM_NO_ERR;
}

dom_exception _dom_html_document_get_url(dom_html_document *doc,
		dom_string **url)
{
	*url = dom_string_ref(doc->url);

	return DOM_NO_ERR;
}

dom_exception _dom_html_document_get_body(dom_html_document *doc,
		struct dom_html_element **body)
{
	dom_exception exc = DOM_NO_ERR;

	if (doc->body != NULL) {
		*body = doc->body;
	} else {
		dom_element *node;
		dom_nodelist *nodes;
		uint32_t len;

		exc = dom_document_get_elements_by_tag_name(doc,
				doc->elements[DOM_HTML_ELEMENT_TYPE_BODY],
				&nodes);
		if (exc != DOM_NO_ERR) {
			return exc;
		}

		exc = dom_nodelist_get_length(nodes, &len);
		if (exc != DOM_NO_ERR) {
			dom_nodelist_unref(nodes);
			return exc;
		}

		if (len == 0) {
			exc = dom_document_get_elements_by_tag_name(doc,
					doc->elements[DOM_HTML_ELEMENT_TYPE_FRAMESET],
					&nodes);
			if (exc != DOM_NO_ERR) {
				return exc;
			}
			exc = dom_nodelist_get_length(nodes, &len);
			if (exc != DOM_NO_ERR) {
				dom_nodelist_unref(nodes);
				return exc;
			}
			if(len == 0) {
				dom_nodelist_unref(nodes);
				return DOM_NO_ERR;
			}
		}

		exc = dom_nodelist_item(nodes, 0, (void *) &node);
		dom_nodelist_unref(nodes);
		if (exc != DOM_NO_ERR) {
			return exc;
		}

		*body = (dom_html_element *)node;
		dom_node_unref(node);
	}

	return exc;
}

dom_exception _dom_html_document_set_body(dom_html_document *doc,
		struct dom_html_element *body)
{
	doc->body = body;
	return DOM_NO_ERR;
}

/**
 * Callback for creating the images collection
 *
 * \param node          The dom_node_internal object
 * \param ctx           The dom_html_document object (void *)
 * \return DOM_NO_ERR on success, appropriate dom_exception on failure.
 */
bool images_callback(struct dom_node_internal *node, void *ctx)
{
	dom_html_document *doc = ctx;
	if(node->type == DOM_ELEMENT_NODE &&
			dom_string_caseless_isequal(node->name,
				doc->elements[DOM_HTML_ELEMENT_TYPE_IMG])) {
		return true;
	}
	return false;
}

dom_exception _dom_html_document_get_images(dom_html_document *doc,
		struct dom_html_collection **col)
{
	dom_element *root;
	dom_exception err;
	err = dom_document_get_document_element(doc, &root);
	if (err != DOM_NO_ERR)
		return err;

	return _dom_html_collection_create(doc, (dom_node_internal *) root, 
			images_callback, doc, col);
}

bool applet_callback(struct dom_node_internal * node, void *ctx)
{
	dom_html_document *doc = ctx;
	if(node->type == DOM_ELEMENT_NODE &&
			dom_string_caseless_isequal(node->name,
				doc->elements[DOM_HTML_ELEMENT_TYPE_APPLET])) {
		return true;
	}
	return false;
}
/**
 * Callback for creating the applets collection
 *
 * \param node          The dom_node_internal object
 * \param ctx           The dom_html_document object (void *)
 * \return true if node is an applet object
 */
bool applets_callback(struct dom_node_internal *node, void *ctx)
{
	dom_html_document *doc = ctx;
	if(node->type == DOM_ELEMENT_NODE &&
			dom_string_caseless_isequal(node->name,
				doc->elements[DOM_HTML_ELEMENT_TYPE_OBJECT])) {
		uint32_t len = 0;
		dom_html_collection *applets;
		if (_dom_html_collection_create(ctx, node,
				applet_callback, ctx, &applets) != DOM_NO_ERR)
			return false;
		dom_html_collection_get_length(applets, &len);
		dom_html_collection_unref(applets);
		if(len != 0)
			return true;
	}
	return false;
}

dom_exception _dom_html_document_get_applets(dom_html_document *doc,
		struct dom_html_collection **col)
{
	dom_element *root;
	dom_exception err;
	err = dom_document_get_document_element(doc, &root);
	if (err != DOM_NO_ERR)
		return err;

	return _dom_html_collection_create(doc, (dom_node_internal *) root, 
			applets_callback, doc, col);
}

/**
 * Callback for creating the links collection
 *
 * \param node          The dom_node_internal object
 * \param ctx           The dom_html_document object (void *)
 * \return DOM_NO_ERR on success, appropriate dom_exception on failure.
 */
bool links_callback(struct dom_node_internal *node, void *ctx)
{
	dom_html_document *doc = ctx;
	if(node->type == DOM_ELEMENT_NODE &&
			(dom_string_caseless_isequal(node->name,
				doc->elements[DOM_HTML_ELEMENT_TYPE_A]) ||
			dom_string_caseless_isequal(node->name,
				doc->elements[DOM_HTML_ELEMENT_TYPE_AREA]))
			 ) {
		bool has_value = false;
		dom_exception err;

		err = dom_element_has_attribute(node,
				doc->memoised[hds_href], &has_value);
		if(err !=DOM_NO_ERR)
			return err;

		if(has_value)
			return true;
	}
	return false;
}

dom_exception _dom_html_document_get_links(dom_html_document *doc,
		struct dom_html_collection **col)
{
	dom_element *root;
	dom_exception err;
	err = dom_document_get_document_element(doc, &root);
	if (err != DOM_NO_ERR)
		return err;

	return _dom_html_collection_create(doc, (dom_node_internal *) root,
			links_callback, doc, col);
}

static bool __dom_html_document_node_is_form(dom_node_internal *node,
		void *ctx)
{
	dom_html_document *doc = (dom_html_document *)node->owner;

	UNUSED(ctx);

	return dom_string_caseless_isequal(node->name,
			doc->elements[DOM_HTML_ELEMENT_TYPE_FORM]);
}

dom_exception _dom_html_document_get_forms(dom_html_document *doc,
		struct dom_html_collection **col)
{
	dom_html_collection *result;
	dom_element *root;
	dom_exception err;

	err = dom_document_get_document_element(doc, &root);
	if (err != DOM_NO_ERR)
		return err;

	err = _dom_html_collection_create(doc, (dom_node_internal *) root, 
			__dom_html_document_node_is_form, NULL, &result);
	if (err != DOM_NO_ERR) {
		dom_node_unref(root);
		return err;
	}

	dom_node_unref(root);

	*col = result;

	return DOM_NO_ERR;
}

/**
 * Callback for creating the anchors collection
 *
 * \param node          The dom_node_internal object
 * \param ctx           The dom_html_document object (void *)
 * \return DOM_NO_ERR on success, appropriate dom_exception on failure.
 */
bool anchors_callback(struct dom_node_internal *node, void *ctx)
{
	dom_html_document *doc = ctx;
	if(node->type == DOM_ELEMENT_NODE &&
			dom_string_caseless_isequal(node->name,
				doc->elements[DOM_HTML_ELEMENT_TYPE_A])) {
		bool has_value = false;
		dom_exception err;

		err = dom_element_has_attribute(node,
				doc->memoised[hds_name], &has_value);
		if(err !=DOM_NO_ERR)
			return err;

		if(has_value)
			return true;
	}
	return false;
}

dom_exception _dom_html_document_get_anchors(dom_html_document *doc,
		struct dom_html_collection **col)
{
	dom_element *root;
	dom_exception err;
	err = dom_document_get_document_element(doc, &root);
	if (err != DOM_NO_ERR)
		return err;

	return _dom_html_collection_create(doc, (dom_node_internal *) root,
			anchors_callback, doc, col);
}

dom_exception _dom_html_document_get_cookie(dom_html_document *doc,
		dom_string **cookie)
{
	UNUSED(doc);
	UNUSED(cookie);
	/*todo implement this after updating client interface */
	return DOM_NOT_SUPPORTED_ERR;
}

dom_exception _dom_html_document_set_cookie(dom_html_document *doc,
		dom_string *cookie)
{
	UNUSED(doc);
	UNUSED(cookie);

	/*todo implement this after updating client interface */
	return DOM_NOT_SUPPORTED_ERR;
}

dom_exception _dom_html_document_open(dom_html_document *doc)
{
	UNUSED(doc);

	/*todo implement this after updating client interface */
	return DOM_NOT_SUPPORTED_ERR;
}

dom_exception _dom_html_document_close(dom_html_document *doc)
{
	UNUSED(doc);
	/*todo implement this after updating client interface */
	return DOM_NOT_SUPPORTED_ERR;
}

dom_exception _dom_html_document_write(dom_html_document *doc,
		dom_string *text)
{
	UNUSED(doc);
	UNUSED(text);

	/*todo implement this after updating client interface */
	return DOM_NOT_SUPPORTED_ERR;
}

dom_exception _dom_html_document_writeln(dom_html_document *doc,
		dom_string *text)
{
	UNUSED(doc);
	UNUSED(text);

	/*todo implement this after _dom_html_document_write */
	return DOM_NOT_SUPPORTED_ERR;
}

dom_exception _dom_html_document_get_elements_by_name(dom_html_document *doc,
		dom_string *name, struct dom_nodelist **list)
{
	UNUSED(doc);
	UNUSED(name);
	UNUSED(list);
	/*todo implement after updating core nodelist interface */
	return DOM_NOT_SUPPORTED_ERR;
}

