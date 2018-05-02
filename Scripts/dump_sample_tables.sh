#!/bin/sh
# Select relevant sample users and dump to a file.

TOP_USER_COUNT=1000
RANDOM_USER_COUNT=1000

DATABASE_HOST=db_delayed
DATABASE_USER=pp-export

OUTPUT_PATH=/var/www/html/

sql() {
    mysql osu -sN -h "${DATABASE_HOST}" -u "${DATABASE_USER}" -e "SELECT('$1'); $2; SELECT CONCAT('✓ Completed with ', ROW_COUNT(), ' rows.');"
}

dump() {
    echo "Dumping $1..."
    path=${output_folder}/${1}.sql
    mysqldump --single-transaction -h "${DATABASE_HOST}" -u "${DATABASE_USER}" osu $1 --where="${2:-1=1}" > ${path}
    echo "✓ Completed with $(stat -c%s "${path}") bytes."
}

case "$1" in
    osu)
        mode_index=0
        suffix="_osu"
        table_suffix=""
        name="osu!"
        ;;
    taiko)
        mode_index=1
        suffix="_taiko"
        table_suffix=$suffix
        name="osu!taiko"
        ;;
    catch)
        mode_index=2
        suffix="_fruits"
        table_suffix=$suffix
        name="osu!catch"
        ;;
    mania)
        mode_index=3
        suffix="_mania"
        table_suffix=$suffix
        name="osu!mania"
        ;;
esac

sample_users_table="sample_users${table_suffix}"
sample_beatmapsets_table="sample_beatmapsets${table_suffix}"
sample_beatmaps_table="sample_beatmaps${table_suffix}"

date=$(date +"%Y_%m_%d")
output_folder="${date}_performance${suffix}"

# WHERE clause to exclude invalid beatmaps
beatmap_set_validity_check="approved > 0 AND deleted_at IS NULL"
beatmap_validity_check="approved > 0 AND deleted_at IS NULL"
user_validity_check="user_warnings = 0 AND user_type != 1"

echo
echo "Creating dump for $name (${output_folder})"
echo

# create sample users table

sql "Creating sample_users table"         "DROP TABLE IF EXISTS ${sample_users_table}; CREATE TABLE ${sample_users_table} ( user_id INT PRIMARY KEY );"
sql "Creating sample_beatmapsets table"   "DROP TABLE IF EXISTS ${sample_beatmapsets_table}; CREATE TABLE ${sample_beatmapsets_table} ( beatmapset_id INT PRIMARY KEY );"
sql "Creating sample_beatmaps table"      "DROP TABLE IF EXISTS ${sample_beatmaps_table}; CREATE TABLE ${sample_beatmaps_table} ( beatmap_id INT PRIMARY KEY );"

sql "Populating top users.."        "INSERT IGNORE INTO ${sample_users_table} SELECT user_id FROM osu_user_stats${table_suffix} ORDER BY rank_score desc LIMIT $TOP_USER_COUNT"
sql "Populating random users.."     "INSERT IGNORE INTO ${sample_users_table} SELECT user_id FROM osu_user_stats WHERE rank_score_index > 0 ORDER BY RAND(1) LIMIT $RANDOM_USER_COUNT;"
sql "Removing restricted users.."   "DELETE FROM ${sample_users_table} WHERE ${sample_users_table}.user_id NOT IN (SELECT user_id FROM phpbb_users WHERE ${user_validity_check});"
sql "Populating beatmapsets.."      "INSERT IGNORE INTO ${sample_beatmapsets_table} SELECT beatmapset_id FROM osu_beatmapsets WHERE ${beatmap_set_validity_check};"
sql "Populating beatmaps.."         "INSERT IGNORE INTO ${sample_beatmaps_table} SELECT beatmap_id FROM osu_beatmaps WHERE ${beatmap_validity_check} AND beatmapset_id IN (SELECT beatmapset_id FROM ${sample_beatmapsets_table});"

mkdir -p ${output_folder}

echo

# user stats (using sample users retrieved above)
dump "osu_scores${table_suffix}_high"   "user_id IN (SELECT user_id FROM ${sample_users_table})"
dump "osu_user_stats${table_suffix}"    "user_id IN (SELECT user_id FROM ${sample_users_table})"

# beatmap tables (we only care about ranked/approved/loved beatmaps)
dump "osu_beatmapsets"                  "beatmapset_id IN (SELECT beatmapset_id FROM ${sample_beatmapsets_table})"
dump "osu_beatmaps"                     "beatmap_id IN (SELECT beatmap_id FROM ${sample_beatmaps_table})"

# beatmap difficulty tables (following same ranked/approved/loved rule as above, plus only for the intended game mode)
dump "osu_beatmap_difficulty"           "mode = $mode_index AND beatmap_id IN (SELECT beatmap_id FROM ${sample_beatmaps_table})"
dump "osu_beatmap_difficulty_attribs"   "mode = $mode_index AND beatmap_id IN (SELECT beatmap_id FROM ${sample_beatmaps_table})"

# misc tables
dump "osu_difficulty_attribs"
dump "osu_beatmap_performance_blacklist"
dump "osu_counts"

echo

#clean up
sql "Dropping sample_users table.." "DROP TABLE ${sample_users_table}"
sql "Dropping sample_beatmaps table.." "DROP TABLE ${sample_beatmaps_table}"
sql "Dropping sample_beatmapsets table.." "DROP TABLE ${sample_beatmapsets_table}"

echo
echo "Compressing.."

tar -cvjSf ${output_folder}.tar.bz2 ${output_folder}/*
rm -r ${output_folder}

mv ${output_folder}.tar.bz2 ${OUTPUT_PATH}/

echo
echo "All done!"
echo
