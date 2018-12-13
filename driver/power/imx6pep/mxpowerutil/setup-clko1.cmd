:: Copyright (c) Microsoft Corporation. All rights reserved.
:: Licensed under the MIT License.

echo Setting GPIO0 to CLKO1
mxpowerutil padmux gpio0 0
mxpowerutil padctl gpio0 -spd max -dse 50 -sre fast