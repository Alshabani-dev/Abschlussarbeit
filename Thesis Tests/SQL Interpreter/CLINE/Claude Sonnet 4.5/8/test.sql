# Test script for MiniSQL
CREATE TABLE users (id, name, age);
INSERT INTO users VALUES (1, Alice, 30);
INSERT INTO users VALUES (2, Bob, 25);
INSERT INTO users VALUES (3, Charlie, 35);
SELECT * FROM users;
SELECT * FROM users WHERE age = 25;

CREATE TABLE products (id, name, price);
INSERT INTO products VALUES (101, Laptop, 999);
INSERT INTO products VALUES (102, Mouse, 25);
SELECT * FROM products;
