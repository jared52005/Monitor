# Global Coloring Rules
Monitor devices are using this set of coloring rules:

 * **Request** Pink / Black
 * **Positive Response** Green / Black
 * **Negative Response** Red / White
   * **If NR for 22** Dark Orange / Grey
   * **If NR for 78** Yellow / Black
   * **If NR for 33C4 / 33C5** Yellow / Black
 * **Unknown** Black / Pink

Info column formatting  
 * **Request** `[22F190] - RDBLI; VIN` = First 3 bytes of request, Service ID (22 = RDBLI) Subfunciton ID (F190 = VIN)
 * **Positive Response** `[62F190] - RDBLI; VIN` = First 3 bytes of request, Service ID (22 = RDBLI) Subfunciton ID (F190 = VIN)
 * **Negative Response** `[7F2233] - RDBLI; Security Requested` = Service ID (22 = RDBLI) Negative Response code (33 = Security)