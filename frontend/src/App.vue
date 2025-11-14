<script setup lang="ts">
import "./style.css"
import MotorCard from "@/components/motorConfiguration/motorCard.vue";
</script>

<template>
  <h1>You did it!</h1>
  <p>
    Visit <a href="https://vuejs.org/" target="_blank" rel="noopener">vuejs.org</a> to read the
    documentation
  </p>
  <div class="flex flex-row min-h-screen justify-center items-start">
    <MotorCard/>
  </div>
</template>

<style scoped></style>
