DROP FUNCTION IF EXISTS json_members;
DROP FUNCTION IF EXISTS json_array;
DROP FUNCTION IF EXISTS json_object;
DROP FUNCTION IF EXISTS json_values;
CREATE FUNCTION json_members RETURNS STRING SONAME 'lib_mysqludf_json.so';
CREATE FUNCTION json_array RETURNS STRING SONAME 'lib_mysqludf_json.so';
CREATE FUNCTION json_object RETURNS STRING SONAME 'lib_mysqludf_json.so';
CREATE FUNCTION json_values RETURNS STRING SONAME 'lib_mysqludf_json.so';
