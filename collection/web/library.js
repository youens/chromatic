'use strict';
for(const button of document.querySelectorAll('[data-filter]')){
 button.addEventListener('click',()=>{
  let count=0;
  for(const filter of document.querySelectorAll('[data-filter]'))filter.setAttribute('aria-pressed',String(filter===button));
  for(const card of document.querySelectorAll('[data-category]')){
   card.hidden=button.dataset.filter!=='all'&&card.dataset.category!==button.dataset.filter;
   if(!card.hidden)count++;
  }
  document.querySelector('#game-count').textContent=String(count).padStart(2,'0')+(count===1?' GAME':' GAMES');
 });
}
