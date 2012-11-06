
1. OG�LNIE

  Braa s�u�y do odpytywania host�w po SNMP - w odr�nieniu jednak od snmpget
z pakietu net-snmp, braa mo�e odpytywa� dziesi�tki czy nawet setki i tysi�ce
host�w jednocze�nie, i to w dodatku pracuj�c jako jeden proces. Dzi�ki temu
zu�ywana jest niewielka ilo�� zasob�w systemowych a samo odpytywanie (czy w
zasadzie, skanowanie) przebiega BARDZO szybko.

  Braa jest wyposa�ony w w�asny stos SNMP, nie wymaga wi�c �adnych bibliotek
SNMP takich jak net-snmp. Sama implementacja jest bardzo prowizoryczna,
obs�uguje tylko kilka typ�w danych i z pewno�ci� nie mo�e by� okre�lona
jako 'zgodna ze standardami'. Zosta�a zaprojektowana tak aby by�a szybka -
i jest szybka. Z tego powodu (i oczywi�cie z powodu mojego lenistwa), nie
ma analizatora j�zyka ASN.1 w braa - MUSISZ zna� numeryczne warto�ci OID'�w
o kt�re chcesz odpytywa�. (np. .1.3.6.1.2.1.1.5.0, a nie system.sysName.0).

2. WYMAGANIA
  
  * system z rodziny *IX w kt�rym dzia�aj� gniazda BSD i kilka istotnych
    syscalli z POSIX'a
  * du�a tablica ARP - je�li chcesz odpytywa� tysi�ce host�w, potrzebujesz
    systemu kt�ry b�dzie w stanie pomie�ci� tysi�ce wpis�w ARP... na przyk�ad
    zainteresuj si� gc_thresh w linuksie...

  * dobrze jest mie� te� zainstalowany gdzie� (niekoniecznie na tej samej
    maszynie) pe�ny pakiet SNMP - braa przyjmuje tylko numeryczne OIDy,
    wi�c czasem mo�e si� okaza�, �e snmptranslate to ca�kiem przydatne
    narz�dzie...

   Braa nie do ko�ca jest 'portable' - w ka�dym razie, zosta�o przetestowane pod
  systemami:
  
   * Linux (shaerrawedd 2.4.19-xfs #7 Fri Oct 4 18:18:38 CEST 2002 i686 unknown)
   * FreeBSD (venom 4.6.2-RELEASE-p10 FreeBSD 4.6.2-RELEASE-p10 #0: Tue Mar 25
           12:59:45 CET 2003     root@venom:/usr/src/sys/compile/VENOM-3  i386)
   * OpenBSD (pantera 3.3 PANTERA#0 i386)
   * SunOS (atlantis 5.9 Generic_112233-04 sun4u sparc SUNW,Ultra-5_10)


3. INSTALACJA

  * przyjrzyj si� Makefile i odkomentuj ustawienia dla Twojego systemu
  * zr�b 'make'
  * skopiuj binark� 'braa' w Twoje ulubione miejsce
   
4. U�YCIE

  Braa potrzebuje listy zapyta�, okre�lonych w poni�szej sk�adni:
  [community@]host[:port]:oid[/id]
  Gdzie: host          to adres IP hosta z kt�rym si� po��czy�
         oid           to OID o kt�ry spyta�
		 port          port UDP z kt�rym si� po��czy� (domy�lnie: 161)
         community     to community jakim si� zidentyfikowa�, domy�lnie
                       public
		 id            identyfikator zapytania (przydatny przy u�ywaniu
		               braa w skryptach; sam w sobie nie wp�ywa na proces
					   zapytania)
		 
  Przyk�ady:
     192.168.12.30:.1.3.6.1.2.1.1.5.0
     private@192.168.31.1:.1.3.6.1.2.1.1.6.0
  Mo�na poda� tak�e ca�y zakres host�w, powiedzmy:
     10.253.100.1-10.253.105.254:.1.3.6.1.2.1.2.2.1.10.1
  to to samo co:
     10.253.100.1:.1.3.6.1.2.1.2.2.1.10.1
     10.253.100.2:.1.3.6.1.2.1.2.2.1.10.1
     10.253.100.3:.1.3.6.1.2.1.2.2.1.10.1
     ...
     ...


  Je�li potrzebujesz odpyta� hosta (lub zakres host�w) o wi�cej ni� jedn�
  warto�� SNMP, musisz po prostu wpisa� kilka osobnych zapyta�:
     10.253.101.1-10.253.106.1:.1.3.6.1.2.1.10.127.1.1.2.1.1.4
     10.253.101.1-10.253.106.1:.1.3.6.1.2.1.2.2.1.10.1
     10.253.101.1-10.253.106.1:.1.3.6.1.2.1.2.2.1.16.1
     10.253.101.1-10.253.106.1:.1.3.6.1.2.1.10.127.1.1.1.1.6.3
     10.253.101.1-10.253.106.1:.1.3.6.1.2.1.10.127.1.2.2.1.3.2
     10.253.101.1-10.253.106.1:.1.3.6.1.2.1.10.127.1.1.4.1.5.3
  Naturalnie braa wykorzystuje mo�liwo�� wys�ania kilku zapyta� SNMP
  w jednym pakiecie i nigdy nie wy�le wi�cej ni� jeden pakiet (poza
  sytuacjami retransmisji je�li nie ma odpowiedzi) do jednego hosta -
  swoj� drog�, zobacz OGRANICZENIA.

  No, tak jak zosta�o to wspomniane, pierwszy etap to przygotowanie listy
  zapyta�. List� mo�na przekaza� braa na dwa sposoby. Albo zapisa� j� do pliku
  tekstowego, w kt�rym jedno zapytanie == jedna linijka, albo poda� seri�
  opcji -q. Mo�na tak�e miesza� obydwie te metody, za�adowa� kilka zestaw�w
  zapyta� z kilku r�nych plik�w, itd.
  
  A oto w�a�ciwa sk�adnia:
  braa [-a] [-s N] [-r R] [-t T] [-q query [-q query [-q query ...]]] [-f file
       [-f file [-f file ...]]]
       
    WYMAGANE OPCJE:
    -q i -f   to opcje do podawania zapyta�. -q pozwala na dos�owne podanie
              zapytania, -f pozwala na podanie pliku z kt�rego zapytania
              za�adowa�.
    OPCJE ZMIENIAJ�CE SZYBKO�C DZIA�ANIA:
    -s N      okre�la ile host�w pyta� w jednej turze, domy�lnie 50, patrz ni�ej
    -r R      okre�la ilo�� retransmisji na host, domy�lnie 3, patrz ni�ej
    -t T      okre�la timeout w milisekundach, patrz ni�ej
    OPCJE ZMIENIAJ�CE FORMAT WYJ�CIA
    -a        alternatywny format wyj�cia

    Wi�c jak to dzia�a?
  
    Tworzona jest kolejka N host�w i wysy�ane s� do niej zapytania. Braa
  czeka do 200 ms na jakie� odpowiedzi. Je�li co� przyjdzie, hosty kt�re
  odpowiedzia�y s� usuwane z kolejki i kolejne hosty wchodz� w ich miejsce.
  Ilo�� retransmisji dla ka�dego hosta kt�ry nie odpowiedzia� jest nast�pnie
  zwi�kszana - je�li ta ilos� osi�gnie R, host jest r�wnie� usuwany
  z kolejki, zwalniaj�c miejsce dla nast�pnego. Host usuni�ty z kolejki
  nadal mo�e wys�a� odpowied� i ZOSTANIE ona przyj�ta, st�d op�nienia
  pakiet�w s� bez znaczenia, k�opot jedynie w stratach. Ca�y proces
  jest powtarzany dop�ki nie sko�cz� si� hosty do zape�niania nowych
  miejsc w kolejce (tzn. wszyscy zostali ju� odpytani lub s� w trakcie
  odpytywania). Kiedy tak nast�pi, kolejka jest stopniowo zmniejszana,
  i kiedy ju� osi�gnie d�ugo�� 0, braa zamyka wszystkie po��czenia,
  sortuje wyniki, wy�wietla je, wychodzi.
    Po T milisekundach od startu, braa pokazuje wyniki i wychodzi
  natychmiast, niezale�nie od tego ile host�w zosta�o ju� odpytanych,
  a do ilu nie posz�o nawet p� pakietu...

    Jak wida�, algorytm nie jest doskona�y. Jak wspomnia�em, chodzi�o o to,
  �eby by� jak najszybszy. Domy�lne warto�ci dla -s i -r s� ca�kiem w porz�dku,
  ale oczywi�cie zawsze mo�esz je 'podkr�ci�' tak, aby braa chodzi�o jeszcze
  szybciej (ale mniej dok�adnie). Je�li odpytujesz bardzo du�� ilo�� host�w, nie
  zapomnij zwi�kszy� znacz�co -t.

    Wyniki s� wypisywane na stdout, jeden-na-lini�:  
    192.168.1.2:.1.3.6.1.2.1.1.5.0:do-wan.elsat.net.pl
    192.168.1.1:.1.3.6.1.2.1.1.4.0:"Mateusz Golicz MG452-RIPE <mtg@elsat.net.pl>"
    192.168.1.1:.1.3.6.1.2.1.1.5.0:ergsun

    Lub je�li podasz -a, host-na-lini� z wynikami oddzielonymi spacj�, w kolejno�ci
  w kt�rej zosta�y podane zapytania o nie:

    10.253.105.245 358 410 -137 276783656 16315982 4
    10.253.105.244 387 480 -105 6611527 391938 1
    10.253.105.242 371 282 182 0 0 2
    10.253.105.239 373 430 34 0 0 6
    10.253.105.238 354 510 -163 262523537 12319047 4
    
	Przy standardowym trybie wypisywania wynik�w braa mo�e przed ka�dym rezultatem
  zapytania wypisa� jego podany przez u�ytkownika identyfikator; ta opcja mo�e
  by� przydatna do przetwarzania wyj�cia z braa w skryptach. Przyk�adowo, zapytanie o:
    192.168.1.2:.1.3.6.1.2.1.1.5.0/Test
    zwr�ci co� w rodzaju:
	Test/192.168.1.2:.1.3.6.1.2.1.1.5.0:do-wan.elsat.net.pl
    
5. PRZYK�ADY

    braa -f queries.lst \
         -q 192.168.1.1:.1.3.6.1.2.1.1.5.0 \
         -q 192.168.1.1:.1.3.6.1.2.1.1.4.0 \
         -q 192.168.1.2:.1.3.6.1.2.1.1.4.0
    
    braa -s 50 -ar 3 -t 250000 -q 10.253.101.1-10.253.106.1:.1.3.6.1.2.1.10.127.1.1.2.1.1.4 \
                               -q 10.253.101.1-10.253.106.1:.1.3.6.1.2.1.2.2.1.10.1 \
                               -q 10.253.101.1-10.253.106.1:.1.3.6.1.2.1.2.2.1.16.1 \
                               -q 10.253.101.1-10.253.106.1:.1.3.6.1.2.1.10.127.1.1.1.1.6.3 \
                               -q 10.253.101.1-10.253.106.1:.1.3.6.1.2.1.10.127.1.2.2.1.3.2 \
                               -q 10.253.101.1-10.253.106.1:.1.3.6.1.2.1.10.127.1.1.4.1.5.3

5. B�EDY I OGRANICZENIA

  * jedynymi obs�ugiwanymi typami danych s�: integer (gauge, counter, timeticks,
    etc.), string and OID. Oczywi�cie zawsze mo�esz poprawi� braaasn.c/braaasn.h.
  * braa nigdy nie wy�le wi�cej ni� 1500 bajt�w (lub pojedynczy pakiet) do hosta
    w pojedynczej pr�bie. St�d ilo�� zapyta� jakie mo�esz zada� jednemu hostowi
    jest ograniczona. Dodatkowo, kiedy osi�gniesz ten limit podaj�c zbyt
    wiele zapyta�, braa przerwie ca�y proces skanowania. Nie potrafi� okre�li�
    ile dok�adnie wynosi ten limit, po prostu zale�y to od wielu czynnmik�w
    (g��wnie, d�ugo�ci OID'�w), w ka�dym razie, 15 zapyta� na hosta brzmi
    do�� niebezpiecznie i lepiej nie przekraczaj tej liczby.
  * nie da si� u�ywa� nazw domenowych host�w - zawsze musisz podawa� IP.
    c�, to drobiazg, my�l� �e w nast�pnej wersji (o ile b�dzie) zostanie
    to poprawione. 
  * braa prawdopodobnie nie ruszy w �rodowiskach 64-bitowych

6. AUTORZY, PODZIEKOWANIA

  Mateusz 'mteg' Golicz <mtg@elsat.net.pl>

7. LICENCJA

  Pakiet rozpowszechniany jest na zasadach licencji GNU GPL, wersja 2.

Mateusz 'mteg' Golicz <mtg@elsat.net.pl>
