// Copyright (c) Hercules Dev Team, licensed under GNU GPL.
// See the LICENSE file

#include "../common/cbasetypes.h"
#include "../common/strlib.h"
#include "../common/timer.h"
#include "../common/HPMi.h"
#include "../common/malloc.h"
#include "../common/mmo.h"
#include "../common/conf.h"
#include "../config/core.h"
#include "../map/clif.h"
#include "../map/pc.h"
#include "../map/map.h"
#include "../map/itemdb.h"
#include <stdio.h>
#include <stdlib.h>

HPExport struct hplugin_info pinfo = {
	"DB2SQL",		// Plugin name
	SERVER_TYPE_MAP,// Which server types this plugin works with?
	"0.5",			// Plugin version
	HPM_VERSION,	// HPM Version (don't change, macro is automatically updated)
};

struct {
	FILE *fp;
	struct {
		char *p;
		unsigned int len;
	} buf[4];
	char *db_name;
} tosql;
bool torun = false;

int (*itemdb_readdb_libconfig_sub) (config_setting_t *it, int n, const char *source);
int (*h_config_setting_lookup_string) (const config_setting_t *setting, const char *name, const char **value);
int (*h_config_setting_lookup_int) (const config_setting_t *setting, const char *name, int *value);
char* trimbraces(char* str) {
	size_t start;
	size_t end;
	
	if( str == NULL )
		return str;
	
	for( start = 0; str[start] && str[start] == '{'; ++start )
		;
	for( end = strlen(str); start < end && str[end-1] && (str[end-1] == '}' || str[end-1] == '\n'); --end )
		;
	if( start == end )
		*str = '\0';
	else {
		str[end] = '\0';
		memmove(str,str+start,end-start+1);
		trim(str);
	}
	return str;
}
void hstr(const char *str) {
	if( strlen(str) > tosql.buf[3].len ) {
		tosql.buf[3].len = tosql.buf[3].len + strlen(str) + 1000;
		RECREATE(tosql.buf[3].p,char,tosql.buf[3].len);
	}
	safestrncpy(tosql.buf[3].p,str,strlen(str));
	normalize_name(tosql.buf[3].p,"\t\n");
}
int db2sql(config_setting_t *entry, int n, const char *source) {
	struct item_data *it = NULL;
	
	if( (it = itemdb->exists(itemdb_readdb_libconfig_sub(entry,n,source))) ) {
		char e_name[ITEM_NAME_LENGTH*2+1], e_jname[ITEM_NAME_LENGTH*2+1];
		const char *script = NULL;
		char *str;
		int i32;
		unsigned int ui32, job = 0, upper = 0;
		
		SQL->EscapeString(NULL, e_name, it->name);
		SQL->EscapeString(NULL, e_jname, it->jname);
		if( it->script )  { h_config_setting_lookup_string(entry, "Script", &script); hstr(script); str = tosql.buf[3].p; trimbraces(str); if ( strlen(str) > tosql.buf[0].len ) { tosql.buf[0].len = tosql.buf[0].len + strlen(str) + 1000; RECREATE(tosql.buf[0].p,char,tosql.buf[0].len); } SQL->EscapeString(NULL, tosql.buf[0].p, str); }
		if( it->equip_script ) { h_config_setting_lookup_string(entry, "OnEquipScript", &script); hstr(script); str = tosql.buf[3].p; trimbraces(str); if ( strlen(str) > tosql.buf[1].len ) { tosql.buf[1].len = tosql.buf[1].len + strlen(str) + 1000; RECREATE(tosql.buf[1].p,char,tosql.buf[1].len); } SQL->EscapeString(NULL, tosql.buf[1].p, str); }
		if( it->unequip_script ) { h_config_setting_lookup_string(entry, "OnUnequipScript", &script); hstr(script); str = tosql.buf[3].p; trimbraces(str);  if ( strlen(str) > tosql.buf[2].len ) { tosql.buf[2].len = tosql.buf[2].len + strlen(str) + 1000; RECREATE(tosql.buf[2].p,char,tosql.buf[2].len); } SQL->EscapeString(NULL, tosql.buf[2].p, str); }
		
		if( h_config_setting_lookup_int(entry, "Job", &i32) ) // This is an unsigned value, do not check for >= 0
			ui32 = (unsigned int)i32;
		else
			ui32 = UINT_MAX;
		
		job = ui32;
		
		if( h_config_setting_lookup_int(entry, "Upper", &i32) && i32 >= 0 )
			ui32 = (unsigned int)i32;
		else
			ui32 = ITEMUPPER_ALL;

		upper = ui32;
		
		fprintf(tosql.fp,"REPLACE INTO `%s` VALUES ('%u','%s','%s','%u','%u','%u','%u','%u','%u','%u','%u','%u','%u','%u','%u','%u','%u','%u','%u','%u','%u','%s','%s','%s');\n",
				tosql.db_name,it->nameid,e_name,e_jname,it->flag.delay_consume?IT_DELAYCONSUME:it->type,it->value_buy,it->value_sell,it->weight,it->atk,it->matk,it->def,it->range,it->slot,job,upper,it->sex,it->equip,it->wlv,it->elv,it->elvmax,it->flag.no_refine?0:1,it->look,it->script?tosql.buf[0].p:"",it->equip_script?tosql.buf[1].p:"",it->unequip_script?tosql.buf[2].p:"");
	}
	
	return it?it->nameid:0;
}
void totable(void) {
	fprintf(tosql.fp,"#\n"
					"# Table structure for table `%s`\n"
					"#\n"
					"\n"
					"DROP TABLE IF EXISTS `%s`;\n"
					"CREATE TABLE `%s` (\n"
					"   `id` smallint(5) unsigned NOT NULL DEFAULT '0',\n"
					"   `name_english` varchar(50) NOT NULL DEFAULT '',\n"
					"   `name_japanese` varchar(50) NOT NULL DEFAULT '',\n"
					"   `type` tinyint(2) unsigned NOT NULL DEFAULT '0',\n"
					"   `price_buy` mediumint(10) NOT NULL DEFAULT '0',\n"
					"   `price_sell` mediumint(10) NOT NULL DEFAULT '0',\n"
					"   `weight` smallint(5) unsigned NOT NULL DEFAULT '0',\n"
					"   `atk` smallint(5) unsigned NOT NULL DEFAULT '0',\n"
					"   `matk` smallint(5) unsigned NOT NULL DEFAULT '0',\n"
					"   `defence` smallint(5) unsigned NOT NULL DEFAULT '0',\n"
					"   `range` tinyint(2) unsigned NOT NULL DEFAULT '0',\n"
					"   `slots` tinyint(2) unsigned NOT NULL DEFAULT '0',\n"
					"   `equip_jobs` int(12) unsigned NOT NULL DEFAULT '0',\n"
					"   `equip_upper` tinyint(8) unsigned NOT NULL DEFAULT '0',\n"
					"   `equip_genders` tinyint(2) unsigned NOT NULL DEFAULT '0',\n"
					"   `equip_locations` smallint(4) unsigned NOT NULL DEFAULT '0',\n"
					"   `weapon_level` tinyint(2) unsigned DEFAULT NULL,\n"
					"   `equip_level_min` smallint(5) unsigned NOT NULL DEFAULT '0',\n"
					"   `equip_level_max` smallint(5) unsigned NOT NULL DEFAULT '0',\n"
					"   `refineable` tinyint(1) unsigned NOT NULL DEFAULT '0',\n"
					"   `view` smallint(3) unsigned DEFAULT NULL,\n"
					"   `script` text,\n"
					"   `equip_script` text,\n"
					"   `unequip_script` text,\n"
					" PRIMARY KEY (`id`)\n"
					") ENGINE=MyISAM;\n"
					"\n",tosql.db_name,tosql.db_name,tosql.db_name);
}
void do_db2sql(void) {
	if( map->db_use_sql_item_db ) {
		ShowInfo("db2sql: this should not be used with 'db_use_sql_item_db' enabled, skipping...\n");
		return;
	}
	
	/* link */
	itemdb_readdb_libconfig_sub = itemdb->readdb_libconfig_sub;
	itemdb->readdb_libconfig_sub = db2sql;
	/* */
	
	if ((tosql.fp = fopen("sql-files/item_db_re.sql", "wt+")) == NULL) {
		ShowError("itemdb_tosql: File not found \"%s\".\n", "sql-files/item_db_re.sql");
		return;
 	}
	
	tosql.db_name = map->item_db_re_db;
	totable();
	
	memset(&tosql.buf, 0, sizeof(&tosql.buf) );
	
	itemdb->clear(false);
	itemdb->readdb("re/item_db.conf");
	
	fclose(tosql.fp);
	
	if ((tosql.fp = fopen("sql-files/item_db.sql", "wt+")) == NULL) {
		ShowError("itemdb_tosql: File not found \"%s\".\n", "sql-files/item_db.sql");
		return;
 	}
	
	tosql.db_name = map->item_db_db;
	totable();
	
	itemdb->clear(false);
	itemdb->readdb("pre-re/item_db.conf");
	
	fclose(tosql.fp);
	
	/* unlink */
	itemdb->readdb_libconfig_sub = itemdb_readdb_libconfig_sub;
	
	if( tosql.buf[0].p ) aFree(tosql.buf[0].p);
	if( tosql.buf[1].p ) aFree(tosql.buf[1].p);
	if( tosql.buf[2].p ) aFree(tosql.buf[2].p);
	if( tosql.buf[3].p ) aFree(tosql.buf[3].p);
}
CPCMD(db2sql) {
	do_db2sql();
}
void db2sql_arg(char *param) {
	torun = true;
}
HPExport void server_preinit (void) {
	h_config_setting_lookup_string = GET_SYMBOL("config_setting_lookup_string");
	h_config_setting_lookup_int = GET_SYMBOL("config_setting_lookup_int");
	
	SQL = GET_SYMBOL("SQL");
	itemdb = GET_SYMBOL("itemdb");
	map = GET_SYMBOL("map");
	strlib = GET_SYMBOL("strlib");
	iMalloc = GET_SYMBOL("iMalloc");
	
	addArg("--db2sql",false,db2sql_arg,NULL);
	
	map->minimal = true;
}
HPExport void plugin_init (void) {
	HPMi->addCPCommand("server:tools:db2sql",CPCMD_A(db2sql));
}
HPExport void server_online (void) {
	if( torun )
		do_db2sql();
}
