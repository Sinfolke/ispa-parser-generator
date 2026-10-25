SELECT name, age, salary * 2 AS income
FROM employees
WHERE age >= 18 AND salary > 3000
ORDER BY age DESC;