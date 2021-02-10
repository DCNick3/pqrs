package com.google.zxing.qrcode.decoder;

import com.google.gson.Gson;

import java.util.ArrayList;
import java.util.Map;
import java.util.TreeMap;

public class VersionDump {
  public static void main(String[] args) {
    ArrayList<Object> jResult = new ArrayList<>();

    for (int i = 1; i <= 40; i++) {
      Version ver = Version.getVersionForNumber(i);
      int[] alignment = ver.getAlignmentPatternCenters();

      ErrorCorrectionLevel[] levels = new ErrorCorrectionLevel[] {
        ErrorCorrectionLevel.M, ErrorCorrectionLevel.L, ErrorCorrectionLevel.H, ErrorCorrectionLevel.Q
      };

      ArrayList<Map<String, Object>> jEcBlocksList = new ArrayList<>();

      for (ErrorCorrectionLevel level : levels) {
        Version.ECBlocks ecBlocksContainer = ver.getECBlocksForLevel(level);

        ArrayList<Map<String, Object>> jEcBlocks = new ArrayList<>();

        for (Version.ECB block : ecBlocksContainer.getECBlocks()) {
          Map<String, Object> jBlock = new TreeMap<>();
          jBlock.put("count", block.getCount());
          jBlock.put("dataCodewords", block.getDataCodewords());
          jEcBlocks.add(jBlock);
        }

        Map<String, Object> jEcBlocksContainer = new TreeMap<>();

        jEcBlocksContainer.put("ecCodewordsPerBlock", ecBlocksContainer.getECCodewordsPerBlock());
        jEcBlocksContainer.put("ecBlocks", jEcBlocks);

        jEcBlocksList.add(jEcBlocksContainer);
      }

      Map<String, Object> jVersion = new TreeMap<>();
      jVersion.put("alignment", alignment);
      jVersion.put("ecBlocks", jEcBlocksList);

      jResult.add(jVersion);
    }

    Gson gson = new Gson();
    System.out.println(gson.toJson(jResult));
  }
}
