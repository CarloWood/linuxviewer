document.addEventListener("DOMContentLoaded", function() {
  const navArrows = document.querySelectorAll("ul.Nav > li.has-children > a > .Nav__arrow");
  for (let i = 0; i < navArrows.length; i++) {
    navArrows[i].addEventListener("click", function(e) {
      e.preventDefault();
      e.stopPropagation();
      const listItem = e.target.closest(".Nav__item");
      listItem.classList.toggle("Nav__item--open");
    });
  }
});
