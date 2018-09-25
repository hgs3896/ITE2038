USE Pokemon;

# 1. 잡은 포켓몬이 3마리 이상인 트레이너의 이름을 잡은 포켓몬의 수가 많은 순서대로 출력하세요.
SELECT T.name FROM Trainer AS T, CatchedPokemon AS C WHERE T.id = C.owner_id GROUP BY owner_id HAVING count(*) >= 3 ORDER BY count(*) DESC;

# 2. 전체 포켓몬 중 가장 많은 2개의 타입을 가진 포켓몬 이름을 사전순으로 출력하세요.
SELECT name FROM Pokemon AS P JOIN (SELECT type FROM Pokemon GROUP BY type ORDER BY count(*) DESC LIMIT 2) AS TP ON P.type = TP.type ORDER BY name;

# 3. o가 두번째에 들어가는 포켓몬의 이름을 사전순으로 출력하세요.
SELECT name FROM Pokemon WHERE name LIKE '_o%' ORDER BY name;

# 4. 잡은 포켓몬중 레벨이 50이상인 포켓몬의 닉네임을 사전순으로 출력하세요.
SELECT nickname FROM CatchedPokemon WHERE level >= 50 ORDER BY nickname;

# 5. 이름이 6글자인 포켓몬의 이름을 사전순으로 출력하세요.
SELECT name FROM Pokemon WHERE LENGTH(name) = 6 ORDER BY name;

# 6. Blue City 출신의 트레이너를 사전순으로 출력하세요.
SELECT name FROM Trainer WHERE hometown = 'Blue City' ORDER BY name;

# 7. 트레이너의 고향 이름을 중복없이 사전순서대로 출력하세요.
SELECT DISTINCT hometown FROM Trainer ORDER BY hometown;

# 8. Red가 잡은 포켓몬의 평균 레벨을 출력하세요.
SELECT AVG(level) FROM CatchedPokemon, Trainer WHERE owner_id = Trainer.id and name = "Red";

# 9. 잡은 포켓몬 중 닉네임이 T로 시작하지 않는 포켓몬의 닉네임을 사전순으로 출력하세요.
SELECT nickname FROM CatchedPokemon WHERE nickname NOT LIKE 'T%' ORDER BY nickname;

# 10. 잡은 포켓몬 중 레벨이 50이상이고 owner_id가 6이상인 포켓몬의 닉네임을 사전순으로 출력하세요
SELECT nickname FROM CatchedPokemon WHERE level >= 50 and owner_id >= 6 ORDER BY nickname;

# 11. 포켓몬 도감에 있는 모든 포켓몬의 ID와 이름을 ID에 오름차순으로 정렬해 출력하세요.
SELECT id, name FROM Pokemon ORDER BY id;

# 12. 레벨이 50이하인 잡힌 포켓몬의 닉네임을 레벨에 오름차순으로 정렬해 출력하세요.
SELECT nickname FROM CatchedPokemon WHERE level <= 50 ORDER BY level;

# 13. 상록시티 출신 트레이너가 가진 포켓몬들의 이름과 포켓몬ID를 포켓몬ID의 오름차순으로 정렬해 출력하세요.
SELECT P.name, P.id FROM Trainer as T, Pokemon as P, CatchedPokemon as C WHERE T.hometown = 'Sangnok City' and P.id = C.pid GROUP BY P.id ORDER BY P.id;

# 14. 모든 각 도시의 관장이 가진 포켓몬들 중 물포켓몬들의 별명을 별명에 오름차순으로 정렬해 출력하세요.
SELECT C.nickname FROM Trainer AS T, CatchedPokemon AS C, Pokemon P WHERE T.id = C.owner_id and C.pid = P.id and P.type = "Water" ORDER BY C.nickname;

# 15. 모든 잡힌 포켓몬들 중에서 진화가 가능한 포켓몬의 총 수를 출력하세요
SELECT count(*) FROM Evolution AS E, CatchedPokemon AS C WHERE E.before_id = C.pid;

# 16. 포켓몬 도감에 있는 포켓몬 중 ’Water’타입, ‘Electric’타입, ‘Psychic’ 타입 포켓몬의 총 합을 출력하세요.
SELECT count(*) FROM Pokemon WHERE type in ('Water', 'Electric', 'Psychic');

# 17. 상록시티 출신 트레이너들이 가지고 있는 포켓몬 종류의 갯수를 출력하세요.
SELECT count(DISTINCT C.pid) FROM Trainer AS T, CatchedPokemon AS C WHERE T.id = C.owner_id and T.hometown = "Sangnok City";

# 18. 상록시티 출신 트레이너들이 가지고 있는 포켓몬 중 레벨이 가장 높은 포켓몬의 레벨을 출력하세요.
SELECT max(C.level) FROM Trainer AS T, CatchedPokemon AS C WHERE T.id = C.owner_id and T.hometown = "Sangnok City";

# 19. 상록시티의 리더가 가지고 있는 포켓몬 타입의 갯수를 출력하세요.
SELECT count(DISTINCT P.type) FROM Gym AS G, CatchedPokemon AS C, Pokemon P WHERE G.leader_id = C.owner_id and C.pid = P.id and G.city = "Sangnok City";

# 20. 상록시티 출신 트레이너의 이름과 각 트레이너가 잡은 포켓몬 수를 잡은 포켓몬 수에 대해 오름차순 순서대로 출력하세요.
SELECT T.name, count(*) FROM Trainer AS T, CatchedPokemon AS C WHERE T.id = C.owner_id and T.hometown = "Sangnok City" GROUP BY T.id ORDER BY count(*);

# 21. 포켓몬의 이름이 알파벳 모음으로 시작하는 포켓몬의 이름을 출력하세요.
SELECT name FROM Pokemon WHERE name REGEXP '^(a|e|i|o|u)[:print:]*';

# 22. 포켓몬의 타입과 해당 타입을 가지는 포켓몬의 수가 몇 마리인지를 출력하세요 수에대해 오름차순으로 출력하세요. ( 같은 수를 가진 경우 타입 명의 사전식 오름차순으로 정렬하세요.)
SELECT type, count(*) FROM Pokemon GROUP BY type ORDER BY count(*), type;

# 23. 잡힌 포켓몬 중 레벨이 10이하인 포켓몬을 잡은 트레이너의 이름을 중복없이 사전순으로 출력하세요.
SELECT DISTINCT T.name FROM CatchedPokemon AS C, Trainer AS T WHERE C.owner_id = T.id and C.level <= 10 ORDER BY T.name;

# 24. 각 시티의 이름과 해당 시티의 고향 출신 트레이너들이 잡은 포켓몬들의 평균 레벨을 평균 레벨에 오름차순으로 정렬해서 출력하세요.
SELECT T.hometown, AVG(CP.level) FROM CatchedPokemon AS CP, Trainer AS T WHERE CP.owner_id = T.id GROUP BY T.hometown ORDER BY AVG(CP.level);

# 25. 상록시티 출신 트레이너와 브라운시티 출신 트레이너가 공통으로 잡은 포켓몬의 이름을 사전순으로 출력하세요. ( 중복은 제거할 것 )
SELECT P.name FROM Pokemon AS P, CatchedPokemon AS C, Trainer AS T WHERE P.id = C.pid and C.owner_id = T.id and T.hometown IN ('Sangnok City', 'Brown City') GROUP BY P.id HAVING COUNT(DISTINCT T.hometown) >= 2 ORDER BY P.name;

# 26. 잡힌 포켓몬 중 닉네임에 공백이 들어가는 포켓몬의 이름을 사전식 내림차순으로 출력하세요.
SELECT P.name FROM Pokemon AS P, CatchedPokemon AS C WHERE P.id = C.pid and C.nickname LIKE '%\ %' ORDER BY P.name DESC;

# 27. 포켓몬을 4마리 이상 잡은 트레이너의 이름과 해당 트레이너가 잡은 포켓몬 중 가장 레벨이 높은 포켓몬의 레벨을 트레이너의 이름에 사전순으로 정렬해서 출력하세요.
SELECT T.name, MAX(C.level) FROM Trainer AS T, CatchedPokemon AS C WHERE T.id = C.owner_id GROUP BY T.id HAVING COUNT(*) >= 4 ORDER BY T.name;

# 28. ‘Normal’타입 혹은 ‘Electric’타입의 포켓몬을 잡은 트레이너의 이름과 해당 포켓몬의 평균 레벨을 구한 평균 레벨에 오름차순으로 정렬해서 출력하세요.
SELECT T.name, AVG(CP.level) FROM Trainer AS T, CatchedPokemon AS CP, Pokemon AS P WHERE T.id = CP.owner_id and CP.pid = P.id and P.type IN ('Normal', 'Electric') GROUP BY T.id ORDER BY AVG(CP.level);

# 29. 152번 포켓몬의 이름과 이를 잡은 트레이너의 이름, 출신 도시의 설명(description)을 잡힌 포켓몬의 레벨에 대한 내림차순 순서대로 출력하세요 ( 포켓몬의 레벨은 출력하지 않음 )
SELECT P.name, T.name, C.description FROM Pokemon AS P, Trainer AS T, City AS C, CatchedPokemon AS CP WHERE P.id = 152 and P.id = CP.pid and T.id = CP.owner_id and T.hometown = C.name ORDER BY CP.level DESC;

# 30. 포켓몬 중 3단 진화가 가능한 포켓몬의 ID와 해당 포켓몬의 이름을 1단 진화 형태 포켓몬의 이름, 2단 진화 형태 포켓몬의 이름, 3단 진화 형태 포켓몬의 이름을 ID의 오름차순으로 출력하세요.
SELECT P1.id, P1.name, P2.name, P3.name FROM Evolution AS E0, Evolution AS E1, Pokemon AS P1, Pokemon AS P2, Pokemon AS P3 WHERE E0.after_id = E1.before_id and P1.id = E0.before_id and P2.id = E1.before_id and P3.id = E1.after_id ORDER BY P1.id;

# 31. 포켓몬 ID가 두자리 수인 포켓몬의 이름을 사전순으로 출력하세요.
SELECT P.name FROM Pokemon AS P WHERE P.id BETWEEN 10 and 99 ORDER BY P.name;

# 32. 어느 트레이너에게도 잡히지 않은 포켓몬의 이름을 사전순으로 출력하세요.
SELECT name FROM Pokemon AS C WHERE id NOT IN (SELECT pid FROM CatchedPokemon) ORDER BY name;

# 33. 트레이너 Matis가 잡은 포켓몬들의 레벨의 총합을 출력하세요.
SELECT SUM(CP.level) FROM Trainer AS T, CatchedPokemon AS CP WHERE T.id = CP.owner_id and T.name = 'Matis';

# 34. 체육관 관장들의 이름을 사전순으로 출력하세요.
SELECT T.name FROM Trainer AS T, Gym G WHERE T.id = G.leader_id ORDER BY T.name;

# 35. 가장 많은 비율의 포켓몬 타입과 그 비율을 백분율로 출력하세요.
SELECT P.type, 100 * COUNT(*) / COUNT_TABLE.cnt FROM (SELECT COUNT(*) AS cnt FROM CatchedPokemon) AS COUNT_TABLE, Pokemon AS P, CatchedPokemon AS CP WHERE P.id = CP.pid GROUP BY P.type ORDER BY COUNT(*) DESC LIMIT 1;

# 36. 닉네임이 A로 시작하는 포켓몬을 잡은 트레이너의 이름을 사전순으로 출력하세요.
SELECT T.name FROM CatchedPokemon AS CP, Trainer AS T WHERE CP.owner_id = T.id and CP.nickname LIKE 'A%' ORDER BY T.name;

# 37. 잡은 포켓몬 레벨의 총합이 가장 높은 트레이너의 이름과 그 총합을 출력하세요.
SELECT T.name, SUM(CP.level) FROM CatchedPokemon AS CP, Trainer AS T WHERE CP.owner_id = T.id GROUP BY T.id ORDER BY SUM(CP.level) DESC LIMIT 1;

# 38. 진화 2단계 포켓몬들의 이름을 사전순으로 출력하세요 (ex. 꼬부기-어니부기-거북왕의 경우 진화 2단계 포켓몬은 어니부기, 2단 진화가 최종 진화 형태인 포켓몬도 출력해야함.)
SELECT name FROM Pokemon WHERE id in ( SELECT after_id FROM Evolution WHERE before_id not in ( SELECT E0.after_id FROM Evolution AS E0, Evolution AS E1 WHERE E0.after_id = E1.before_id ) ) ORDER BY name;

# 39. 동일한 포켓몬을 두 마리 이상 잡은 트레이너의 이름을 사전순으로 출력하세요.
SELECT T.name FROM Trainer AS T, CatchedPokemon AS CP WHERE T.id = CP.owner_id GROUP BY T.id HAVING COUNT(CP.pid) - COUNT(DISTINCT CP.pid) > 0 ORDER BY T.name;

# 40. 출신 도시명과 각 출신 도시별로 트레이너가 잡은 가장 레벨이 높은 포켓몬의 별명을 출신 도시 명의 사전순으로 출력하세요.
SELECT hometown, nickname FROM Trainer, CatchedPokemon WHERE ROW(hometown, level) in (SELECT T.hometown, MAX(CP.level) FROM Trainer AS T, CatchedPokemon AS CP WHERE T.id = CP.owner_id GROUP BY T.hometown) ORDER BY hometown;