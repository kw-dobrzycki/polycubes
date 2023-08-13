Original

|          | test  | don't test |
|----------|-------|------------|
| positive | 7.11  | 5.478      |
| negative | 1.603 | 28.513     |

Using own compareLocalEncodings with fixed square error, spin uses mutliset

|          | test  | don't test |
|----------|-------|------------|
| positive | 5.756 | 5.599      |
| negative | 0.041 | 28.066     |

spin now uses groupcomparev

|          | test | don't test |
|----------|------|------------|
| positive | 0.55 | 0.559      |
| negative | 0.38 | 8.252      |

after bug fixing

|          | test  | don't test |
|----------|-------|------------|
| positive | 1.970 | 1.913      |
| negative | 0.46  | 10.773     |

after loop optim

|          | test  | don't test |
|----------|-------|------------|
| positive | 1.652 | 1.663      |
| negative | 0.030 | 8.406      |

after neighbour locality

|          | test    | don't test |
|----------|---------|------------|
| positive | timeout | timeout    |
| negative | 3.3     | timeout    |

comparing encodings may be slow. change it to a sparse set.

|          | test | don't test |
|----------|------|------------|
| positive | 1.55 | 1.46       |
| negative | 0.06 | 8.58       |

spintest now copies tets

|          | test  | don't test |
|----------|-------|------------|
| positive | 2.203 | 2.03       |
| negative | 0.06  | 8.98       |



no skips
2m   - 87208921

all skips
1.56 - 87183781 (99.97117%)

local skips
3.68 - 86970899 (99.72706%)

inverse skips
1.62 - 87166012 (99.95079%)

population
4.86 - 86641227 (99.34904%)

bounds
29.5 - 72778729 (83.45330%)