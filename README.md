# Fakulta informačních technologií Vysokého učení technického v Brně
## projekt ISA (Síťové a plikace a správa sítí) 2020
## varianta: Filtrující DNS resolver
## autor: David Rubý
***

Implementace projektu v jazyce ```C++```, apolu s základníma síťovýma a jinýma knihovnama.

Překlad probíhal na operačním systému ```Ubuntu 18.04``` za pomocí překladače ```g++ 7.5.0```.

### Funkčnost
Aplikace přeposílá DNS packety přes UDP protokol na zvolený server a výsledek zasílá zpět klientovi. Poslouchá na zvoleném portu. Filteruje domény a request na resolve domény, která se nachází v seznamu filterovaných domén zahuje.

### Obsažené soubory
+ ```dns.cpp```
+ ```manual.pdf```
+ ```README.md```
+ ```Makefile```

### Překlad
Překlad probíhá pomocí souboru ```Makefile```. V adresáři, kde se tento soubor nachází, stačí v terminálu napsat příkaz:

> ```make```

Pro odstranění binárních souborů se použije příkaz:

> ```make clear```

### Spuštění
Spuštění aplikace se provede zadáním následujicího přikazu do terminálu v adresáři, kde se nachází soubor ```dns```

> ```./dns -s \<server> -p \<port> -f \<filter file>```

V případě, že se používá defaultní port 53, nebo port s hodnotou nižší než 1024, je třeba aplikaci spustit jako správce následujicím způsobem:

> ```sudo ./dns -s \<server> -p \<port> -f \<filter file>```

Aplikace se ukončuje pomocí klávesové zkratky ```CTTRL+C```.

### Argumenty
> ```-s \<server>```
 
Udává IP adresu DNS serveru, na který se mají přeposílat DNS požadavky.

> ```-p \<port>```

Udává port, na kterém bude aplikace komunikovat s klientem. Tento Argument není povinný. Při nezadání portu se použije defaultní hodnota ```53```.

> ```-f \<filter file>```

Udává umístění souboru, ve kterém se nachází Filtrované domény.

> ```-h OR --help```

Vypíše nápovědu

Prosím o nepoužívání portu ```56654```, protože se používá pro komunikaci s DNS serverem. Při použití tohoto portu pro komunikaci s klientem celá aplikace selže.

### Filter file
Na každém novém řádku se nachází domény, které se budou filtrovat.
Prázdný řádek se přeskakuje. Přeskakují se i komentáře, které jsou celořádkové a začínají symbolem ```#```. Tyto řádky se ignorují.
