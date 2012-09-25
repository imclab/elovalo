-- |Implements Weber–Fechner law for per weight perception. Using S0
-- of 1, k of 1. See more information:
-- http://en.wikipedia.org/wiki/Weber%E2%80%93Fechner_law
module WeberFechner (weberFechnerTable,cTable) where

import Data.List (intercalate)

humanBits  = 8   -- ^Input value range
pwmBits    = 12  -- ^Output value range

a = log(2^pwmBits-1)/(2^humanBits-1)

weberFechner x = round $ exp (a*x)

weberFechnerTable = map weberFechner [0..2^humanBits-1]

header = "/* Pre-calculated Weber–Fechner table generated by helpers/WeberFechner.hs */"
signature = "const uint16_t weber_fechner_table[] PROGMEM"

cTable = header ++ "\n" ++ signature ++ " = {" ++ list ++ "};"
  where list = intercalate "," $ map show weberFechnerTable

main = putStrLn cTable