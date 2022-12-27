CREATE DATABASE demo1;
CREATE DATABASE demo2;
USE demo1;
CREATE TABLE table_1(colName_1 VARCHAR(10), colName_2 INT, colName_3 INT);
CREATE TABLE table_3(colName_1 VARCHAR(10), colName_2 INT, colName_3 INT);
ALTER TABLE table_1 ADD INDEX (colName_1, colName_2, colName_3);
ALTER TABLE table_1 ADD INDEX (colName_4);
ALTER TABLE table_1 DROP INDEX (colName_2);
SHOW INDEXES;

ALTER TABLE table_3 ADD CONSTRAINT PRIMARY KEY (colName_1);
ALTER TABLE table_3 ADD CONSTRAINT PRIMARY KEY (colName_2);
ALTER TABLE table_3 DROP PRIMARY KEY;
ALTER TABLE table_3 ADD CONSTRAINT PRIMARY KEY (colName_2);
ALTER TABLE table_3 ADD UNIQUE (colName_1, colName_2);
ALTER TABLE table_1 ADD CONSTRAINT PRIMARY KEY (colName_2);
ALTER TABLE table_3 ADD CONSTRAINT fpk_1 FOREIGN KEY (colName_2) REFERENCES table_1 (colName_2);
ALTER TABLE table_3 ADD CONSTRAINT fpk_2 FOREIGN KEY (colName_2) REFERENCES table_1 (colName_1);
ALTER TABLE table_3 DROP UNIQUE (colName_2);
SHOW INDEXES;
DESC table_3;

INSERT INTO table_1 VALUES ('col1', 1, 9), ('col2', 2, 6);
INSERT INTO table_3 VALUES ('col1', 1, 9);
INSERT INTO table_1 VALUES ('col3', 3, 100);
DELETE FROM table_1 WHERE colName_3=100;
UPDATE table_1 SET colName_3=129 WHERE colName_2=2;
SELECT SUM(colName_2) FROM table_1;
SELECT * FROM table_1, table_3 WHERE table_1.colName_2=table_3.colName_2;
