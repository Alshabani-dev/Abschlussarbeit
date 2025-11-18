-- Test script for MiniSQL
-- Create a table
CREATE TABLE users (id, name, age);

-- Insert some data
INSERT INTO users VALUES (1, Alice, 30);
INSERT INTO users VALUES (2, Bob, 25);
INSERT INTO users VALUES (3, Charlie, 30);

-- Query all data
SELECT * FROM users;

-- Query with WHERE clause
SELECT * FROM users WHERE age = 30;

-- Create another table
CREATE TABLE products (id, name, price);

-- Insert products
INSERT INTO products VALUES (101, Laptop, 999);
INSERT INTO products VALUES (102, Mouse, 25);
INSERT INTO products VALUES (103, Keyboard, 75);

-- Query products
SELECT * FROM products;
SELECT * FROM products WHERE name = Mouse;
